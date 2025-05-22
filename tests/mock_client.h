#ifndef TESTS_MOCK_CLIENT_H_
#define TESTS_MOCK_CLIENT_H_

#include "gmock/gmock.h"
#include "client.h" // Adjust path as necessary
#include "google/protobuf/message.h"
#include <sstream>

class MockClient : public Client {
public:
    // Constructor matching Client's, if any, or provide a default one
    // For example, if Client has Client(int max_pkg_len, bool has_checksum)
    MockClient(int max_pkg_len, bool has_checksum) : Client(max_pkg_len, has_checksum) {}
    MockClient() : Client(8192, false) {} // Default if no specific args needed for mock

    MOCK_METHOD(bool, send_msg, (const google::protobuf::Message &msghead, const google::protobuf::Message &msg, std::ostringstream &err), (override));
    MOCK_METHOD(int, net_tcp_send, (std::ostringstream &errmsg), (override));
    MOCK_METHOD(int, net_tcp_recv, (std::ostringstream &errmsg), (override));
    MOCK_METHOD(bool, recv_msg, (google::protobuf::Message **msghead, google::protobuf::Message **msg, bool &complete, std::ostringstream &err), (override));
    // Add MOCK_METHOD for try_connect_to_peer if needed for other tests, though not strictly for RunGroupOnce if client is pre-connected
    MOCK_METHOD(bool, try_connect_to_peer, (const std::string &peer_addr, std::ostringstream &err_msg), (override));
};

#endif // TESTS_MOCK_CLIENT_H_
