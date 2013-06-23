#include "common.h"
#include "flags.h"
#include "pb_master.h"
#include "config.h"
#include "client.h"
#include "robot.h"
#include "robot.pb.h"


int main(int argc, char **argv) {
	google::ParseCommandLineFlags(&argc, &argv, true);
	google::InitGoogleLogging(argv[0]);
	if (!init_robot_config()) {
		return -1;
	}
	RunRobots(cfg_root);

	return 0;
}
