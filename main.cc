#include "common.h"
#include "flags.h"
#include "pb_master.h"
#include "config.h"
#include "client.h"
#include "robot.h"
#include "robot.pb.h"
#include "frame_config_loader.h" // Added include


int main(int argc, char **argv) {
	google::ParseCommandLineFlags(&argc, &argv, true);
	google::InitGoogleLogging(argv[0]);

    // Load frame header configuration
    if (!FLAGS_frameheadconfig.empty()) {
        FrameConfigLoader loader;
        try {
            global_frame_header_config = loader.load_config(FLAGS_frameheadconfig);
            global_frame_header_config_loaded = true;
            LOG(INFO) << "Successfully loaded frame header config from: " << FLAGS_frameheadconfig;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to load frame header config from '" << FLAGS_frameheadconfig
                       << "': " << e.what();
            // Decide if this is a fatal error. For now, let it continue,
            // but encoding will use defaults or fail later if config_loaded is false.
        }
    } else {
        LOG(WARNING) << "No frame header config specified (--frameheadconfig is empty). "
                     << "Frame encoding will use default/hardcoded behavior.";
        // global_frame_header_config_loaded remains false
    }

	if (!init_robot_config()) {
		return -1;
	}
	RunRobots(cfg_root);

	cleanup_robot_config();
	return 0;
}
