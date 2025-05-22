#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "common.h"
#include "pb_master.h"
#include "robot.pb.h"

using google::protobuf::Message;

struct UniqRequest;

// <request_uniq_name, const pbcfg::Body*>
typedef std::map<std::string, const UniqRequest*> UniqNameMap;
typedef UniqNameMap::iterator UniqNameMapIter;

extern pbcfg::CfgRoot cfg_root;
extern int kMaxTotalClientNum;
extern UniqNameMap uniq_name_map;

bool ValidationRobotConfigs(const pbcfg::CfgRoot &cfg);
bool CollectConfigInfos(const pbcfg::CfgRoot &cfg);
bool init_robot_config();

void cleanup_robot_config();


// UniqRequest 针对每一个 uniq_name 记录请求数据信息
struct UniqRequest {
	~UniqRequest() { delete bodymsg; }
	UniqRequest(const pbcfg::Body *bdcfg, const Message *bdmsg)
		: bodycfg(bdcfg), bodymsg(bdmsg) { }

	const pbcfg::Body *bodycfg;
	const Message *bodymsg;
};


#endif // __CONFIG_H__
