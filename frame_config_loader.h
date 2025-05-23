#ifndef FRAME_CONFIG_LOADER_H_
#define FRAME_CONFIG_LOADER_H_

#include "frame_config_types.h"
#include <string>
#include <yaml-cpp/yaml.h> // Include yaml-cpp header

class FrameConfigLoader {
public:
    FrameHeaderConfig load_config(const std::string& filepath);

// Made public for testing purposes
// private: 
public: 
    // Helper methods for parsing nodes
    FrameFieldDataType parse_data_type(const std::string& type_str);
    FrameFieldValueRule parse_value_rule(const YAML::Node& value_node, FrameFieldDef& field_def);
    // Helper to validate field definition (e.g., size consistency with type)
    bool validate_field_def(const FrameFieldDef& field_def);
};

#endif // FRAME_CONFIG_LOADER_H_
