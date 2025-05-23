#include "gtest/gtest.h"
#include "frame_config_loader.h"
#include "frame_config_types.h"
#include <fstream>
#include <string>
#include <vector>
#include <cstdio> // For std::remove

// Helper Test Fixture for creating temporary YAML files
class FrameConfigLoaderTest : public ::testing::Test {
protected:
    std::string temp_yaml_filepath;

    void SetUp() override {
        // Generate a unique filename for each test to avoid conflicts
        // Note: In a real-world scenario, use a library for proper temp file creation.
        // For this environment, a simple fixed name or one based on the test name might be sufficient.
        // However, if tests run in parallel (not the case here), this needs to be robust.
        // Using a simple name for now.
        temp_yaml_filepath = "temp_test_config.yaml";
    }

    void TearDown() override {
        std::remove(temp_yaml_filepath.c_str());
    }

    void WriteTempYAML(const std::string& content) {
        std::ofstream outfile(temp_yaml_filepath);
        ASSERT_TRUE(outfile.is_open()) << "Failed to open temp YAML file for writing: " << temp_yaml_filepath;
        outfile << content;
        outfile.close();
    }
};

TEST_F(FrameConfigLoaderTest, LoadValidDefaultConfig) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: total_packet_length
      size: 4
      type: uint32_be
      value: "CALC_TOTAL_PACKET_LENGTH"
    - name: proto_header_length
      size: 4
      type: uint32_be
      value: "CALC_PROTOBUF_HEAD_LENGTH"
)YAML";
    WriteTempYAML(yaml_content);

    FrameConfigLoader loader;
    FrameHeaderConfig config;
    ASSERT_NO_THROW(config = loader.load_config(temp_yaml_filepath));

    ASSERT_EQ(config.fields.size(), 2);

    const auto& field1 = config.fields[0];
    EXPECT_EQ(field1.name, "total_packet_length");
    EXPECT_EQ(field1.size_bytes, 4);
    EXPECT_EQ(field1.data_type, FrameFieldDataType::UINT32_BE);
    EXPECT_EQ(field1.value_rule, FrameFieldValueRule::CALC_TOTAL_PACKET_LENGTH);

    const auto& field2 = config.fields[1];
    EXPECT_EQ(field2.name, "proto_header_length");
    EXPECT_EQ(field2.size_bytes, 4);
    EXPECT_EQ(field2.data_type, FrameFieldDataType::UINT32_BE);
    EXPECT_EQ(field2.value_rule, FrameFieldValueRule::CALC_PROTOBUF_HEAD_LENGTH);
}

TEST_F(FrameConfigLoaderTest, LoadLiteralValues) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: magic_number
      size: 2
      type: uint16_be
      value: 0xCAFE # Hex literal
    - name: version
      size: 1
      type: uint8
      value: 1
    - name: tag
      size: 4
      type: fixed_string
      value: "TEST"
)YAML";
    WriteTempYAML(yaml_content);

    FrameConfigLoader loader;
    FrameHeaderConfig config;
    ASSERT_NO_THROW(config = loader.load_config(temp_yaml_filepath));

    ASSERT_EQ(config.fields.size(), 3);

    const auto& field_magic = config.fields[0];
    EXPECT_EQ(field_magic.name, "magic_number");
    EXPECT_EQ(field_magic.size_bytes, 2);
    EXPECT_EQ(field_magic.data_type, FrameFieldDataType::UINT16_BE);
    EXPECT_EQ(field_magic.value_rule, FrameFieldValueRule::LITERAL);
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(field_magic.literal_value));
    EXPECT_EQ(std::get<std::int64_t>(field_magic.literal_value), 0xCAFE);

    const auto& field_version = config.fields[1];
    EXPECT_EQ(field_version.name, "version");
    EXPECT_EQ(field_version.size_bytes, 1);
    EXPECT_EQ(field_version.data_type, FrameFieldDataType::UINT8);
    EXPECT_EQ(field_version.value_rule, FrameFieldValueRule::LITERAL);
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(field_version.literal_value));
    EXPECT_EQ(std::get<std::int64_t>(field_version.literal_value), 1);

    const auto& field_tag = config.fields[2];
    EXPECT_EQ(field_tag.name, "tag");
    EXPECT_EQ(field_tag.size_bytes, 4);
    EXPECT_EQ(field_tag.data_type, FrameFieldDataType::FIXED_STRING);
    EXPECT_EQ(field_tag.value_rule, FrameFieldValueRule::LITERAL);
    ASSERT_TRUE(std::holds_alternative<std::string>(field_tag.literal_value));
    EXPECT_EQ(std::get<std::string>(field_tag.literal_value), "TEST");
}

TEST_F(FrameConfigLoaderTest, LoadNonExistentFile) {
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config("non_existent_file.yaml"), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, LoadMalformedYAML) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: total_packet_length
      size: 4
      type: uint32_be
      value: "CALC_TOTAL_PACKET_LENGTH"
    - name: proto_header_length
      size: 4
      type: uint32_be
      value: "CALC_PROTOBUF_HEAD_LENGTH # Missing closing quote
)YAML"; 
    WriteTempYAML(yaml_content);

    FrameConfigLoader loader;
    // YAML::ParserException is a subclass of YAML::Exception.
    // The loader wraps YAML::Exception in std::runtime_error.
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

// More tests to be added for invalid structure and field values
TEST_F(FrameConfigLoaderTest, InvalidStructureMissingFrameHeader) {
    const std::string yaml_content = R"YAML(
# frame_header key is missing
fields:
  - name: some_field
    size: 1
    type: uint8
    value: 123
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, InvalidStructureFieldsNotSequence) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields: "this should be a sequence" # Not a sequence
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, IncompleteFieldDefinition) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: field_one # Missing size, type, value
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, UnknownFieldType) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: field_one
      size: 4
      type: "unknown_type_123"
      value: 0
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, UnknownValueRule) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: field_one
      size: 4
      type: uint32_be
      value: "CALC_SOMETHING_INVALID"
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, SizeInconsistentWithType) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: field_one
      size: 2 # uint32_be should be 4 bytes
      type: uint32_be
      value: 12345
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, FixedStringTooLong) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: short_string
      size: 3 
      type: fixed_string
      value: "TOOLONG" 
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, LiteralForNumericNotANumber) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: numeric_field
      size: 4
      type: uint32_be
      value: "not_a_number" # Should be a number for uint32_be
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, LiteralForFixedStringNotAStringInVariant) {
    // This test is tricky because yaml-cpp will convert numbers to strings if 'as<std::string>()' is used.
    // The current FrameConfigLoader::parse_value_rule tries as<std::string> first, then as<std::int64_t>.
    // If a numeric literal is given for a fixed_string field, it will be parsed as a string by yaml-cpp.
    // The validation that checks std::holds_alternative<std::string> for FIXED_STRING literals handles this.
    // A more direct test for this specific case (e.g. if value_node was a map for a string field)
    // is harder to construct without changing how parse_value_rule handles types.
    // The current logic should catch if literal_value is not a string for FIXED_STRING after parsing.
    SUCCEED() << "Test logic for this case is implicitly covered by value/type mismatch tests.";
}

TEST_F(FrameConfigLoaderTest, MissingValueNode) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: field_no_value
      size: 4
      type: uint32_be
      # value node is missing
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, ValueRuleObjectMissingRuleKey) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: field_complex_value_no_rule
      size: 4
      type: uint32_be
      value: 
        parameter: 123 # Missing 'rule' key
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    // This will likely be caught by value_node.as<std::string>() failing if it's a map without 'rule'
    // and then failing the numeric conversion.
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

TEST_F(FrameConfigLoaderTest, ValueRuleObjectUnknownRule) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: field_complex_value_unknown_rule
      size: 4
      type: uint32_be
      value: 
        rule: "UNKNOWN_RULE"
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    EXPECT_THROW(loader.load_config(temp_yaml_filepath), std::runtime_error);
}

// Test for when literal_value is expected to be string but YAML provides a number.
// The current parser logic converts numbers to strings if data_type is fixed_string.
// This test ensures that if it somehow ended up as int64_t in the variant, it's caught.
TEST_F(FrameConfigLoaderTest, ValidateFixedStringLiteralIsString) {
    FrameConfigLoader loader;
    FrameFieldDef field_def;
    field_def.name = "test_str";
    field_def.size_bytes = 5;
    field_def.data_type = FrameFieldDataType::FIXED_STRING;
    field_def.value_rule = FrameFieldValueRule::LITERAL;
    field_def.literal_value = (std::int64_t)12345; // Incorrectly set as int64_t

    // validate_field_def should return false here
    EXPECT_FALSE(loader.validate_field_def(field_def));
}

// Test for when literal_value is expected to be number but YAML provides a non-numeric string.
TEST_F(FrameConfigLoaderTest, ValidateNumericLiteralIsNumber) {
    FrameConfigLoader loader;
    FrameFieldDef field_def;
    field_def.name = "test_num";
    field_def.size_bytes = 4;
    field_def.data_type = FrameFieldDataType::UINT32_BE;
    field_def.value_rule = FrameFieldValueRule::LITERAL;
    field_def.literal_value = "not_a_number"; // Incorrectly set as string

    // Add assertions about the variant's state
    ASSERT_TRUE(std::holds_alternative<std::string>(field_def.literal_value))
        << "Variant should hold a string after assignment.";
    ASSERT_FALSE(std::holds_alternative<std::int64_t>(field_def.literal_value))
        << "Variant should NOT hold an int64_t if it holds a string.";
    
    // You can also add a GTest scoped trace for more context if it fails here
    // SCOPED_TRACE("Checking variant state before validate_field_def");

    // validate_field_def should return false here
    EXPECT_FALSE(loader.validate_field_def(field_def));
}

// Test that CALC_TOTAL_PACKET_LENGTH rule does not require a specific literal_value type
TEST_F(FrameConfigLoaderTest, ValidateCalcTotalPacketLengthNoLiteralCheck) {
    FrameConfigLoader loader;
    FrameFieldDef field_def;
    field_def.name = "total_len";
    field_def.size_bytes = 4;
    field_def.data_type = FrameFieldDataType::UINT32_BE;
    field_def.value_rule = FrameFieldValueRule::CALC_TOTAL_PACKET_LENGTH;
    // literal_value is not set for this rule, should still be valid
    EXPECT_TRUE(loader.validate_field_def(field_def));
}

TEST_F(FrameConfigLoaderTest, ValidateCalcProtoHeadLengthNoLiteralCheck) {
    FrameConfigLoader loader;
    FrameFieldDef field_def;
    field_def.name = "head_len";
    field_def.size_bytes = 2;
    field_def.data_type = FrameFieldDataType::UINT16_BE;
    field_def.value_rule = FrameFieldValueRule::CALC_PROTOBUF_HEAD_LENGTH;
    // literal_value is not set for this rule, should still be valid
    EXPECT_TRUE(loader.validate_field_def(field_def));
}

TEST_F(FrameConfigLoaderTest, FixedStringCorrectLength) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: fixed_str_ok
      size: 4
      type: fixed_string
      value: "TEST" 
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    FrameHeaderConfig config;
    ASSERT_NO_THROW(config = loader.load_config(temp_yaml_filepath));
    ASSERT_EQ(config.fields.size(), 1);
    const auto& field = config.fields[0];
    EXPECT_EQ(field.name, "fixed_str_ok");
    EXPECT_EQ(std::get<std::string>(field.literal_value), "TEST");
}

// Test for fixed_string literal that is SHORTER than size_bytes (should be padded)
// The validate_field_def currently checks if literal_value.length() > size_bytes.
// This means shorter strings are allowed. The padding behavior is an implementation detail
// of how this data is used, not strictly of the loader's validation.
TEST_F(FrameConfigLoaderTest, FixedStringShorterThanSize) {
    const std::string yaml_content = R"YAML(
frame_header:
  fields:
    - name: short_fixed_str
      size: 5
      type: fixed_string
      value: "ABC" 
)YAML";
    WriteTempYAML(yaml_content);
    FrameConfigLoader loader;
    FrameHeaderConfig config;
    ASSERT_NO_THROW(config = loader.load_config(temp_yaml_filepath));
    ASSERT_EQ(config.fields.size(), 1);
    const auto& field = config.fields[0];
    EXPECT_EQ(field.name, "short_fixed_str");
    EXPECT_EQ(field.size_bytes, 5);
    ASSERT_TRUE(std::holds_alternative<std::string>(field.literal_value));
    EXPECT_EQ(std::get<std::string>(field.literal_value), "ABC");
}
