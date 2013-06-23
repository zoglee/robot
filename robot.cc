#include "robot.h"
#include "config.h"
#include "flags.h"
#include "client.h"

void calc_timeout_responses(const pbcfg::Action &actioncfg,
							const std::vector<std::string> &recved_responses,
							std::vector<std::string> &timeout_responses) {
	// TODO(zog): 优化性能 (目前是O(n^2), 不过由于返回包数量不多, 所以没什么影响)
	for (int i = 0; i < actioncfg.response_size(); i++) {
		int found = 0;
		const std::string &expect = actioncfg.response(i);
		for (int r = 0; r < (int)recved_responses.size(); r++) {
			if (recved_responses[r] == expect) {
				found = 1;
				break;
			}
		}
		if (!found) {
			timeout_responses.push_back(expect);
		}
	}
}

bool is_action_compleated(const pbcfg::Action &actioncfg,
						  const std::vector<std::string> &recved_responses) {
	std::vector<std::string> timeout_responses;
	calc_timeout_responses(actioncfg, recved_responses, timeout_responses);
	return timeout_responses.empty();
}

bool RunGroupOnce(int count,
				  const pbcfg::Group &groupcfg,
				  const pbcfg::Client &clientcfg,
				  Client &client,
				  pbcfg::CsMsgHead &headmsg,
				  std::ostringstream &errmsg) {
	bool complete = false;
	bool is_timeout = false;
	time_t wait_start = 0;
	Message *rsphead = 0, *rspbody = 0;
	std::ostringstream requests_strings;
	std::vector<std::string> recved_responses;
	std::vector<std::string> timeout_responses;
	std::ostringstream net_errmsg;
	std::ostringstream op_errmsg;
	for (int i = 0; i < groupcfg.action_size(); i++) {
		const pbcfg::Action &actioncfg = groupcfg.action(i);

LOG(ERROR) << "Client: [" << clientcfg.uid() << "," << clientcfg.role_time()
<< "], req: " << actioncfg.request_uniq_name(0) << ", count: " << count
<< ", stop_loop: " << actioncfg.stop_loop_count() << ", min_duration: " << actioncfg.min_duration();

		if ((actioncfg.stop_loop_count() > 0) && (count >= actioncfg.stop_loop_count())) {
			// 一次执行 Group 内, 连续执行该 Action 的次数
			continue;
		}

		requests_strings.str("");
		requests_strings << "[";
		for (int r = 0; r < actioncfg.request_uniq_name_size(); r++) {
			const std::string &uniq_name = actioncfg.request_uniq_name(r);
			const UniqRequest *uniqreq = uniq_name_map[uniq_name];
			const pbcfg::Body *bodycfg = uniqreq->bodycfg;
			Message *bodymsg = uniqreq->bodymsg->New();
			if (PB_MASTER.load_text_format_string_message(bodycfg->text(), bodymsg) == -1) {
				errmsg << "load text_format_string, type_name: " << bodymsg->GetTypeName();
				delete bodymsg;
				return false;
			}
			std::string type_name = bodymsg->GetTypeName();
			headmsg.set_msg_type_name(type_name);
			if (!client.send_msg(headmsg, *bodymsg, op_errmsg)) {
				errmsg << "send_msg: " << type_name << ", err: " << op_errmsg.str();
				delete bodymsg;
				return false;
			}
			if (r == 0) {
				requests_strings << type_name;
			} else {
				requests_strings << "," << type_name;
			}
			delete bodymsg;
		}
		requests_strings << "]";

		complete = false;
		is_timeout = false;
		wait_start = time(0);
		recved_responses.clear();
		while(true) {
			if ((actioncfg.timeout() > 0) && (time(0) - wait_start > actioncfg.timeout())) {
				is_timeout = true;
				calc_timeout_responses(actioncfg, recved_responses, timeout_responses);
				break;
			}

			if (client.net_tcp_send(net_errmsg) == -1) {
				errmsg << "req:" << requests_strings.str() << ", err: " << net_errmsg.str();
				return false;
			}
			if (client.net_tcp_recv(net_errmsg) == -1) {
				errmsg << "req:" << requests_strings.str() << ", err: " << net_errmsg.str();
				return false;
			}
			if (!client.recv_msg(&rsphead, &rspbody, complete, op_errmsg)) {
				errmsg << "recv_msg, after req:" << requests_strings.str() << "err: " << op_errmsg.str();
				return false;
			}
			if (!complete) {
				usleep(10000);
				continue;
			}
			recved_responses.push_back(rspbody->GetTypeName());
//LOG(ERROR) << "complate: " << rspbody->GetTypeName();
//if (rspbody->GetTypeName() == "ISeer20Comm.ack_errcode_t") {
//	LOG(ERROR) << rsphead->Utf8DebugString();
//	LOG(ERROR) << rspbody->Utf8DebugString();
//}

			// TODO(zog): log response
			delete rsphead;
			delete rspbody;
			rsphead = 0;
			rspbody = 0;

			if (is_action_compleated(actioncfg, recved_responses)) {
				break;
			}
		}

		// 如果设置了最少等待时长, 则必须等到时间
		int32_t dur = (time(0) - wait_start) * 1000;
		if ((actioncfg.min_duration() > 0) && (dur < actioncfg.min_duration())) {
			int32_t usleepdur = (actioncfg.min_duration() - dur) * 1000;
			usleep(usleepdur);
		}

		if (is_timeout) {
			std::ostringstream timeout_string;
			for (int i = 0; i < (int)timeout_responses.size(); i++) {
				if (i == 0) {
					timeout_string << "[" << timeout_responses[i];
				} else {
					timeout_string << "," << timeout_responses[i];
				}
			}
			timeout_string << "]";
			errmsg << "requests: " << requests_strings.str() << " ==> timeout_responses: " << timeout_string.str();
			return false;
		}
	}
	return true;
}

void RobotClientWorker(gpointer data, gpointer user_data) {
	const pbcfg::Group *groupcfg = (const pbcfg::Group *)user_data;
	int client_index = GLIB_POINTER_TO_INT(data) - 1;
	const pbcfg::Client &clientcfg = groupcfg->client(client_index);
	//LOG(ERROR) << "Client-" << groupcfg->name() << ":" << clientcfg.uid() << " started";
	
	std::ostringstream errmsg;
	Client client(groupcfg->max_pkg_len(), groupcfg->has_checksum());
	if (!client.try_connect_to_peer(groupcfg->peer_addr(), errmsg)) {
		LOG(ERROR) << "Error RobotClientWorker, Client-" << groupcfg->name()
			<< ":[" << clientcfg.uid() << ", " << clientcfg.role_time() << "]"
			", cannot connect to peer: " << groupcfg->peer_addr() << ", err: " << errmsg.str();
		return ;
	}

	// TODO(zog): 通用包头设置
	pbcfg::CsMsgHead headmsg;
	headmsg.set_uid(clientcfg.uid());
	headmsg.set_role_tm(clientcfg.role_time());
	headmsg.set_ret(0);

	int count = 0;
	while (count < groupcfg->loop_count()) {
		// TODO(zog): prepare (支持用shell或代码构造测试环境, eg: 把相关服的uid的所在地图都设置成10001)
		if (!RunGroupOnce(count, *groupcfg, clientcfg, client, headmsg, errmsg)) {
			LOG(ERROR) << "Error RunGroupOnce, Client-" << groupcfg->name()
				<< ":[" << clientcfg.uid() << ", " << clientcfg.role_time() << "]: " << errmsg.str();
			return ;
		}
		count++;
	}

	//LOG(ERROR) << "Client-" << groupcfg->name() << ":" << clientcfg.role_time() << " finished"; 
}

void RobotGroupWorker(gpointer data, gpointer user_data) {
	const pbcfg::Group *groupcfg = (const pbcfg::Group *)data;
	LOG(ERROR) << "Group-" << groupcfg->name() << " started";

	// start robot threads
	GThreadPool *thread_pool
		= CreateThreadsPool(groupcfg->client_count(), &RobotClientWorker, gpointer(groupcfg));
	for (int i = 0; i < groupcfg->client_count(); i++) {
		g_thread_pool_push(thread_pool, GLIB_INT_TO_POINTER(i+1), NULL);
	}

	// wait for all robot threads finish (compleate all actions or error)
	g_thread_pool_free(thread_pool, FALSE, TRUE);
	thread_pool = NULL;
LOG(ERROR) << "Group-" << groupcfg->name() << " finished!";
}

void RunRobots(const pbcfg::CfgRoot &cfg) {
	// start robot group threads
	GThreadPool *thread_pool = CreateThreadsPool(cfg.group_config_size(), &RobotGroupWorker);
	for (int i = 0; i < cfg.group_config_size(); i++) {
		const pbcfg::Group &groupcfg = cfg.group_config(i);
		g_thread_pool_push(thread_pool, gpointer(&groupcfg), NULL);
	}

	// wait for all robot group threads being finished (compleate all actions or error)
	g_thread_pool_free(thread_pool, FALSE, TRUE);
	thread_pool = NULL;
	LOG(ERROR) << "RunRobots finished!";
}

// GThreadPool *g_thread_pool_new (
// GFunc func, // a function to execute in the threads of the new thread pool
// gpointer user_data, // user data that is handed over to func every time it is called
// gint max_threads, // the maximal number of threads to execute concurrently
// 					 // in the new thread pool, -1 means no limit
// gboolean exclusive, // should this thread pool be exclusive?
// GError **error // return location for error, or NULL
// );
// Returns : the new GThreadPool

// gboolean g_thread_pool_push (GThreadPool *pool, gpointer data, GError **error);
// pool : a GThreadPool
// data : a new task for pool
// error : return location for error, or NULL
// Returns : TRUE on success, FALSE if an error occurred
GThreadPool *CreateThreadsPool(uint32_t max_threads, threadpool_func_t fn, gpointer user_data) {
	GError *err = 0;
	GThreadPool *thread_pool = g_thread_pool_new(fn, user_data, max_threads, TRUE, &err);
	if(!thread_pool) {
		LOG(ERROR) << "g_thread_pool_new() err: " << err->message;
		return 0;
	}

	return thread_pool;
}
