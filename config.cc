#include "config.h"
#include "flags.h"


pbcfg::CfgRoot cfg_root;
int kMaxTotalClientNum = 10000;
UniqNameMap uniq_name_map;

// Define global frame header config variables
FrameHeaderConfig global_frame_header_config;
bool global_frame_header_config_loaded = false;


bool ValidationRobotConfigs(const pbcfg::CfgRoot &cfg) {
	int total_client = 0;
	std::set<uint64_t> total_client_set;
	
	// check pbcfg::Group
	for (int i = 0; i < cfg.group_config_size(); i++) {
		const pbcfg::Group &groupcfg = cfg.group_config(i);
		total_client += groupcfg.client_count(); // 记录总的客户端数量, 供后续判断

		// 本群的 client_size() 必须大于等于 client_count(这样才够安排), 否则 robot 拒绝启动
		if (groupcfg.client_size() < groupcfg.client_count()) {
			LOG(ERROR) << "Config Error: number of configed "
				"groupcfg.client(" << groupcfg.client_size() << ")"
				" < needed groupcfg.client_count(" << groupcfg.client_count() << ")";
			return false;
		}

		// 不同群的 client 不允许有相同的 (high32(uid) | low32(role_time)), 否则拒绝启动
		for (int c = 0; c < groupcfg.client_size(); c++) {
			const pbcfg::Client &client = groupcfg.client(c);
			uint64_t key = client.uid();
			key = (key << 32) | uint32_t(client.role_time());
			if (total_client_set.count(key) > 0) {
				LOG(ERROR) << "Config Error: Duplicated client: (" << client.uid()
					<< ", " << client.role_time() << ")"; 
				return false;
			}
			total_client_set.insert(key);
		}

		// 任何 Action 中的 request 不允许没有对应的 uniq_name,
		for (int j = 0; j < groupcfg.action_size(); j++) {
			const pbcfg::Action &action = groupcfg.action(j);
			for (int k = 0; k < action.request_uniq_name_size(); k++) {
				const std::string &request_uniq_name = action.request_uniq_name(k);
				if (uniq_name_map.count(request_uniq_name) == 0) {
					LOG(ERROR) << "Config Error: request_uniq_name(" << request_uniq_name
						<< ") is nofound in body configs";
					return false;
				}
			}
		}
	}

	// 总客户端数量(对应线程数量) 不能超过robot的硬限制
	if (total_client > kMaxTotalClientNum) {
		LOG(ERROR) << "Config Error: need too many client: " << total_client
			<< " (> max: " << kMaxTotalClientNum << ")";
		return false;
	}
	
	return true;
}

void cleanup_robot_config() {
	for (UniqNameMapIter it = uniq_name_map.begin(); it != uniq_name_map.end(); ++it) {
		delete it->second;
	}
	uniq_name_map.clear();
}

bool CollectConfigInfos(const pbcfg::CfgRoot &cfg) {
	// 如果发现任何 uniq_name 对应的配置中的 type_name 无法创建消息, 那么 robot 会拒绝启动
	for (int i = 0; i < cfg.body_size(); i++) {
		const pbcfg::Body &bodycfg = cfg.body(i);
		if (uniq_name_map.count(bodycfg.uniq_name()) > 0) {
			LOG(ERROR) << "Duplicated uniq_name: " << bodycfg.uniq_name();
			return false;
		}

		// 任何 Action 中的 request 必须都能被 create_message,
		Message *bodymsg = PB_MASTER.create_message(bodycfg.type_name());
		if (!bodymsg) {
			LOG(ERROR) << "Config Error: request(uniq_name:" << bodycfg.uniq_name()
				<< ", type_name:" << bodycfg.type_name() << ") cannot be create_message";
			return false;
		}

		uniq_name_map[bodycfg.uniq_name()] = new UniqRequest(&bodycfg, bodymsg);
	}

	return true;
}

bool init_robot_config() {
	cfg_root.Clear();
	if (PB_MASTER.load_text_format_message(FLAGS_configfullpath, &cfg_root) == -1) {
		LOG(ERROR) << "Failed load robot config file: " << FLAGS_configfullpath;
		return false;
	}
	// LOG(ERROR) << cfg_root.Utf8DebugString();
	for (int i = 0; i < cfg_root.proto_path_size(); i++) {
		if (!PB_MASTER.import_path(cfg_root.proto_path(i))) {
			return false;
		}
	}
	if (!CollectConfigInfos(cfg_root)) {
		return false;
	}
	if (!ValidationRobotConfigs(cfg_root)) {
		return false;
	}
	return true;
}
