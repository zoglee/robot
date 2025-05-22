#include "common.h"
#include "pb_master.h"
#include "robot.pb.h"
#include "client.h" // Added to declare Client for RunGroupOnce

typedef void (*threadpool_func_t)(gpointer data, gpointer user_data);

void RobotClientWorker(gpointer data, gpointer user_data);
void RobotGroupWorker(gpointer data, gpointer user_data);
void RunRobots(const pbcfg::CfgRoot &cfg);
GThreadPool *CreateThreadsPool(uint32_t max_threads, threadpool_func_t fn, gpointer user_data=0);

// Declaration for RunGroupOnce - assumed signature based on test
bool RunGroupOnce(int groupid, const pbcfg::Group &config, const pbcfg::Client &client_cfg, Client &client, pbcfg::CsMsgHead &head, std::ostringstream &err_msg);
