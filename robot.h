#include "common.h"
#include "pb_master.h"
#include "robot.pb.h"

typedef void (*threadpool_func_t)(gpointer data, gpointer user_data);

void RobotClientWorker(gpointer data, gpointer user_data);
void RobotGroupWorker(gpointer data, gpointer user_data);
void RunRobots(const pbcfg::CfgRoot &cfg);
GThreadPool *CreateThreadsPool(uint32_t max_threads, threadpool_func_t fn, gpointer user_data=0);
