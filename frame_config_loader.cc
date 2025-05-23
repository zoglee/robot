#include "frame_config_loader.h"
#include <fstream>  // For checking file existence (optional good practice)
#include <iostream> // For error messages
#include <stdexcept> // Added as std::runtime_error is used

// --- String to Enum Mapping Helpers (can be static or in an anonymous namespace) ---
namespace { // Anonymous namespace for internal linkage

FrameFieldDataType string_to_data_type(const std::string& type_str) {
    if (type_str == "uint8") return FrameFieldDataType::UINT8;
    if (type_str == "int8") return FrameFieldDataType::INT8;
    if (type_str == "uint16_le") return FrameFieldDataType::UINT16_LE;
    if (type_str == "uint16_be") return FrameFieldDataType::UINT16_BE;
    // ... Add all other types from FrameFieldDataType enum ...
    if (type_str == "int16_le") return FrameFieldDataType::INT16_LE;
    if (type_str == "int16_be") return FrameFieldDataType::INT16_BE;
    if (type_str == "uint32_le") return FrameFieldDataType::UINT32_LE;
    if (type_str == "uint32_be") return FrameFieldDataType::UINT32_BE;
    if (type_str == "int32_le") return FrameFieldDataType::INT32_LE;
    if (type_str == "int32_be") return FrameFieldDataType::INT32_BE;
    if (type_str == "uint64_le") return FrameFieldDataType::UINT64_LE;
    if (type_str == "uint64_be") return FrameFieldDataType::UINT64_BE;
    if (type_str == "int64_le") return FrameFieldDataType::INT64_LE;
    if (type_str == "int64_be") return FrameFieldDataType::INT64_BE;
    if (type_str == "fixed_string") return FrameFieldDataType::FIXED_STRING;
    throw std::runtime_error("Unknown data type: " + type_str);
}

// Basic validation: size consistency. More checks can be added.
bool check_size_consistency(FrameFieldDataType type, int size_bytes) {
    switch (type) {
        case FrameFieldDataType::UINT8:
        case FrameFieldDataType::INT8:
            return size_bytes == 1;
        case FrameFieldDataType::UINT16_LE:
        case FrameFieldDataType::UINT16_BE:
        case FrameFieldDataType::INT16_LE:
        case FrameFieldDataType::INT16_BE:
            return size_bytes == 2;
        // ... Add all other fixed-size types ...
        case FrameFieldDataType::UINT32_LE:
        case FrameFieldDataType::UINT32_BE:
        case FrameFieldDataType::INT32_LE:
        case FrameFieldDataType::INT32_BE:
            return size_bytes == 4;
        case FrameFieldDataType::UINT64_LE:
        case FrameFieldDataType::UINT64_BE:
        case FrameFieldDataType::INT64_LE:
        case FrameFieldDataType::INT64_BE:
            return size_bytes == 8;
        case FrameFieldDataType::FIXED_STRING:
            return size_bytes > 0; // Or any other rule for strings
        default:
            return false; // Should not happen if all types covered
    }
}

} // end anonymous namespace

FrameFieldDataType FrameConfigLoader::parse_data_type(const std::string& type_str) {
    return string_to_data_type(type_str);
}

FrameFieldValueRule FrameConfigLoader::parse_value_rule(const YAML::Node& value_node, FrameFieldDef& field_def) {
    if (!value_node) { // Value node might be optional if all values are calculated
        throw std::runtime_error("Missing 'value' for field: " + field_def.name);
    }
    
    std::string value_str = value_node.as<std::string>();
    if (value_str == "CALC_TOTAL_PACKET_LENGTH") {
        return FrameFieldValueRule::CALC_TOTAL_PACKET_LENGTH;
    } else if (value_str == "CALC_PROTOBUF_HEAD_LENGTH") {
        return FrameFieldValueRule::CALC_PROTOBUF_HEAD_LENGTH;
    } else {
        // It's a literal value
        field_def.value_rule = FrameFieldValueRule::LITERAL;
        // Try to parse as int64_t first for numeric types, then as string
        // YAML-CPP automatically handles hex (0x...) and octal (0...) string conversions to numbers
        try {
            if (field_def.data_type != FrameFieldDataType::FIXED_STRING) {
                 // Check if it looks like a hex string, yaml-cpp's as<int64_t> handles this.
                 field_def.literal_value = value_node.as<std::int64_t>();
            } else {
                field_def.literal_value = value_str; // Store as string for FIXED_STRING
            }
        } catch (const YAML::Exception& e) {
             // If not a number (or not FIXED_STRING), it's an error or needs specific handling
             // For FIXED_STRING, we want the raw string.
             if (field_def.data_type == FrameFieldDataType::FIXED_STRING) {
                 field_def.literal_value = value_str;
             } else {
                 throw std::runtime_error("Invalid literal value '" + value_str + "' for field '" + field_def.name + "': " + e.what());
             }
        }
        return FrameFieldValueRule::LITERAL;
    }
}
   
bool FrameConfigLoader::validate_field_def(const FrameFieldDef& field_def) {
        if (!check_size_consistency(field_def.data_type, field_def.size_bytes)) {
            // Consider throwing an exception or logging an error
            std::cerr << "Error: Size mismatch for field '" << field_def.name 
                      << "'. Type " << static_cast<int>(field_def.data_type) 
                      << " requires a different size than " << field_def.size_bytes << " bytes." << std::endl;
            return false;
        }
        if (field_def.data_type == FrameFieldDataType::FIXED_STRING) {
            if (!std::holds_alternative<std::string>(field_def.literal_value) && field_def.value_rule == FrameFieldValueRule::LITERAL) {
                 std::cerr << "Error: Literal value for FIXED_STRING field '" << field_def.name << "' is not a string." << std::endl;
                 return false;
            }
            if (field_def.value_rule == FrameFieldValueRule::LITERAL && std::get<std::string>(field_def.literal_value).length() > static_cast<size_t>(field_def.size_bytes)) {
                 std::cerr << "Error: Literal string for FIXED_STRING field '" << field_def.name << "' is longer than specified size_bytes." << std::endl;
                 return false;
            }
        } else if (field_def.value_rule == FrameFieldValueRule::LITERAL) { // Numeric types
            // Check if the variant currently holds a string. If so, it's an error for a numeric type.
            if (std::get_if<std::string>(&field_def.literal_value)) {
                std::cerr << "Error: Literal value for numeric field '" << field_def.name 
                          << "' is a string, but a numeric value was expected." << std::endl;
                return false;
            }
            // Optional: Add a further check to ensure it IS an int64_t if it wasn't a string.
            // else if (!std::holds_alternative<std::int64_t>(field_def.literal_value)) {
            //     std::cerr << "Error: Literal value for numeric field '" << field_def.name 
            //               << "' is not a recognizable int64_t after parsing." << std::endl;
            //     return false;
            // }
        }
        // Add more validation as needed
        return true;
   }

FrameHeaderConfig FrameConfigLoader::load_config(const std::string& filepath) {
    // Optional: Check if file exists
    std::ifstream f(filepath.c_str());
    if (!f.good()) {
        throw std::runtime_error("Frame header config file not found: " + filepath);
    }
    f.close();

    YAML::Node config_yaml = YAML::LoadFile(filepath);
    FrameHeaderConfig config_data;

    if (!config_yaml["frame_header"] || !config_yaml["frame_header"]["fields"]) {
        throw std::runtime_error("Invalid YAML structure: Missing 'frame_header' or 'frame_header.fields'");
    }

    const YAML::Node& fields_node = config_yaml["frame_header"]["fields"];
    if (!fields_node.IsSequence()) {
         throw std::runtime_error("'frame_header.fields' is not a sequence");
    }

    for (const auto& field_yaml_node : fields_node) {
        FrameFieldDef field_def;
        if (!field_yaml_node["name"] || !field_yaml_node["size"] || !field_yaml_node["type"] || !field_yaml_node["value"]) {
            throw std::runtime_error("Incomplete field definition in YAML. Required: name, size, type, value.");
        }
        field_def.name = field_yaml_node["name"].as<std::string>();
        field_def.size_bytes = field_yaml_node["size"].as<int>();
        field_def.data_type = parse_data_type(field_yaml_node["type"].as<std::string>());
        
        // ParseValueRule also sets literal_value if rule is LITERAL
        field_def.value_rule = parse_value_rule(field_yaml_node["value"], field_def); 

        if (!validate_field_def(field_def)) {
            throw std::runtime_error("Invalid field definition for: " + field_def.name);
        }
        config_data.fields.push_back(field_def);
    }
    return config_data;
}
