#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "robot.h"         // Adjust path
#include "config.h"        // Adjust path
#include "pb_master.h"     // Adjust path
#include "robot.pb.h"      // Adjusted path for generated protobuf header
#include "mock_client.h"
#include <sstream>

// Global UniqNameMap, as used by the application.
// Tests need to manage its state carefully.
extern UniqNameMap uniq_name_map; // Use extern to link against the definition in config.cc

class RunGroupOnceTest : public ::testing::Test {
protected:
    MockClient mock_client; // Uses default constructor: Client(8192, false)
    pbcfg::Group group_config;
    pbcfg::Client client_config_proto; // Protobuf message for client configuration
    pbcfg::CsMsgHead head_message;    // Protobuf message for request head
    std::ostringstream error_stream;

    // Member to hold the dynamically created UniqRequest for cleanup
    UniqRequest* test_uniq_req = nullptr;
    const pbcfg::Body* test_body_cfg = nullptr; // Points to a static or member instance if needed
    google::protobuf::Message* test_proto_msg = nullptr; // Dynamically allocated for UniqRequest

    RunGroupOnceTest() {
        // Constructor: PB_MASTER initialization and other one-time setup can go here if needed,
        // but prefer SetUp for per-test setup.
        // Ensure PB_MASTER is initialized, which might involve loading .proto files
        // or ensuring descriptors are linked. For this project, linking robot.pb.cc
        // makes its descriptors available. google.protobuf.Empty is standard.
        // If PB_MASTER.import_path() was needed, it could be called here or in a TestEnvironment.
    }

    void SetUp() override {
        // Clean up any state from previous tests (important for globals like uniq_name_map)
        if (test_uniq_req) {
            delete test_uniq_req; // test_proto_msg is deleted by UniqRequest's destructor
            test_uniq_req = nullptr;
            test_proto_msg = nullptr; 
        }
        uniq_name_map.clear(); // Clear the global map

        // Setup a basic UniqRequest for the action
        // We need a static or long-lived pbcfg::Body instance to point to.
        // For simplicity, make it a member of the test fixture or a static local.
        static pbcfg::Body static_dummy_body_cfg; // Static to ensure lifetime for UniqRequest
        static_dummy_body_cfg.set_uniq_name("TestRequest1");
        static_dummy_body_cfg.set_type_name("google.protobuf.Empty");
        test_body_cfg = &static_dummy_body_cfg;

        test_proto_msg = PB_MASTER.create_message("google.protobuf.Empty");
        ASSERT_NE(test_proto_msg, nullptr) << "PB_MASTER failed to create google.protobuf.Empty.";
        
        test_uniq_req = new UniqRequest(test_body_cfg, test_proto_msg); // UniqRequest takes ownership of test_proto_msg
        uniq_name_map["TestRequest1"] = test_uniq_req;

        // --- Setup group_config ---
        group_config.set_name("TestGroup");
        group_config.set_loop_count(1); // Important for RunGroupOnce to execute the action once
        group_config.set_max_pkg_len(8192);
        group_config.set_has_checksum(false);
        group_config.set_peer_addr("dummy_server:1234"); // Required by RunGroupOnce

        pbcfg::Action* action = group_config.add_action();
        action->add_request_uniq_name("TestRequest1"); // Matches uniq_name_map setup
        action->set_timeout(1); // 1 second
        action->add_response("google.protobuf.Empty"); // Expected response type

        // --- Setup client_config_proto (protobuf message for robot config) ---
        client_config_proto.set_uid(123);
        client_config_proto.set_role_time(456);
        // client_config_proto.set_server_addr("dummy_server:1234"); // Set if Client needs it explicitly

        // --- Setup head_message (protobuf message for request head) ---
        head_message.set_uid(client_config_proto.uid());
        head_message.set_role_tm(client_config_proto.role_time());
        head_message.set_ret(0); // Default to success for request head
        // head_message.set_flow(...); // Set if needed
    }

    void TearDown() override {
        // Clean up dynamically allocated UniqRequest.
        // The message within test_uniq_req (test_proto_msg) is deleted by UniqRequest's destructor.
        if (test_uniq_req) {
            delete test_uniq_req;
            test_uniq_req = nullptr;
            test_proto_msg = nullptr; // Already deleted by UniqRequest
        }
        uniq_name_map.clear(); // Clear the global map to prevent state leakage
    }
};

TEST_F(RunGroupOnceTest, BasicSuccessPath) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::SetArgPointee;
    using ::testing::Invoke;

    // Expectations for MockClient
    // EXPECT_CALL(mock_client, try_connect_to_peer(group_config.peer_addr(), _))
    //    .WillOnce(Return(true)); // Removed as per instruction

    EXPECT_CALL(mock_client, send_msg(_, _, _))
        .WillOnce(Return(true));

    // net_tcp_send might be called multiple times if the send buffer isn't cleared.
    // Assuming a simple case where one call is enough.
    EXPECT_CALL(mock_client, net_tcp_send(_))
        .WillOnce(Return(0)); // 0 for success

    EXPECT_CALL(mock_client, net_tcp_recv(_))
        .WillOnce(Return(0)); // 0 for success, assuming some data received that leads to recv_msg

    EXPECT_CALL(mock_client, recv_msg(_, _, _, _))
        .WillOnce(Invoke([&](google::protobuf::Message** head, google::protobuf::Message** body, bool& complete, std::ostringstream& err_stream) {
            // Create and populate the mock head response
            *head = PB_MASTER.create_message("pbcfg.CsMsgHead"); 
            if (!*head) {
                err_stream << "Mock: PB_MASTER failed to create CsMsgHead for mock response.";
                return false;
            }
            pbcfg::CsMsgHead* res_head = static_cast<pbcfg::CsMsgHead*>(*head);
            res_head->CopyFrom(head_message); // Copy original request head fields
            res_head->set_ret(0); // Set success for the response
            // res_head->set_flow(...); // Set if needed for response validation logic

            // Create and populate the mock body response
            *body = PB_MASTER.create_message("google.protobuf.Empty"); 
            if (!*body) {
                err_stream << "Mock: PB_MASTER failed to create google.protobuf.Empty for mock response.";
                delete *head; // Clean up allocated head
                *head = nullptr;
                return false;
            }
            // No fields to set for Empty.
            
            complete = true; // Indicate the message is complete
            return true;     // Indicate recv_msg succeeded
        }));
    
    bool result = RunGroupOnce(0, group_config, client_config_proto, mock_client, head_message, error_stream);
    
    ASSERT_TRUE(result) << "RunGroupOnce failed: " << error_stream.str();
    ASSERT_TRUE(error_stream.str().empty()) << "Error stream from RunGroupOnce was not empty: " << error_stream.str();
}

// Main function is removed as gtest_main is already linked.
