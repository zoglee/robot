#ifndef FRAME_CONFIG_TYPES_H_
#define FRAME_CONFIG_TYPES_H_

#include <string>
#include <vector>
#include <variant> // For std::variant in FrameFieldDef
#include <cstdint> // For fixed-width integer types like std::int64_t

// Defines the data type of a field in the frame header
enum class FrameFieldDataType {
    UINT8,
    INT8,
    UINT16_LE, // Little Endian
    UINT16_BE, // Big Endian
    INT16_LE,
    INT16_BE,
    UINT32_LE,
    UINT32_BE,
    INT32_LE,
    INT32_BE,
    UINT64_LE,
    UINT64_BE,
    INT64_LE,
    INT64_BE,
    FIXED_STRING, // For fixed-size string fields (can also be used for raw byte sequences)
    // Potentially others like FLOAT_LE, FLOAT_BE, DOUBLE_LE, DOUBLE_BE if needed later
};

// Defines how the value of a frame header field is determined
enum class FrameFieldValueRule {
    LITERAL,                 // Value is provided directly in the configuration
    CALC_TOTAL_PACKET_LENGTH,  // Value is calculated as the total length of the packet/frame (e.g., header + body)
    CALC_PROTOBUF_HEAD_LENGTH, // Value is calculated as X + CsMsgHead.ByteSize(), where X is a configured constant (e.g. size of other fixed fields before CsMsgHead)
    // Future rules: CALC_CHECKSUM, etc.
};

// Defines a single field in the frame header
struct FrameFieldDef {
    std::string name;          // Name of the field (for identification/debugging)
    int size_bytes;            // Size of this field in bytes
    FrameFieldDataType data_type; // Data type of the field
    FrameFieldValueRule value_rule; // Rule for determining the field's value

    // Used if value_rule is LITERAL.
    // For numeric types, int64_t can hold all integer literal values.
    // For FIXED_STRING, std::string holds the literal characters or can represent hex data.
    std::variant<std::int64_t, std::string> literal_value; 

    // Optional: parameters for specific rules, e.g., for CALC_PROTOBUF_HEAD_LENGTH
    // For CALC_PROTOBUF_HEAD_LENGTH, this could be the 'X' value (size of preceding fixed fields).
    // For CALC_TOTAL_PACKET_LENGTH, this might be an offset or adjustment.
    // For now, keeping it simple. Can be extended with another std::variant or specific members.
    // For example:
    // int rule_param; // e.g. X for CALC_PROTOBUF_HEAD_LENGTH, or base for checksum

    // Optional: parameters for specific rules, e.g., for checksums later
    // std::vector<std::string> checksum_over_parts; // Names of fields to include in checksum
    // std::string checksum_algorithm;             // e.g., "CRC32", "SUM8"
};

// Represents the overall configuration for a frame header (and potentially trailer)
struct FrameHeaderConfig {
    std::vector<FrameFieldDef> fields; // Ordered list of fields in the header
    // std::vector<FrameFieldDef> trailer_fields; // For future use if trailers are needed
};

#endif // FRAME_CONFIG_TYPES_H_
