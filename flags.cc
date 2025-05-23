#include "flags.h"

#include <stdio.h>

#define FLAGS_MUST_GT_0(flag) \
	bool flag##Validation(const char *flagname, int32_t value) { \
		if (value <= 0) { \
			printf("Invalid value for --%s: %d\n", flagname, (int)value); \
			return false; \
		} \
		return true; \
	} \
	static const bool __check__##flag = google::RegisterFlagValidator(&FLAGS_##flag, &flag##Validation)

DEFINE_int32(loopcount, 10, "loop execute count (will depressed by loop_time)");
FLAGS_MUST_GT_0(loopcount);

DEFINE_int32(looptime, 10, "loop time (seconds)");
FLAGS_MUST_GT_0(looptime);

DEFINE_string(configfullpath, "./proto/robot.pbconf", "fullpath of the config file");
DEFINE_string(frameheadconfig, "frame_header.yaml", "Path to the frame header YAML configuration file");
