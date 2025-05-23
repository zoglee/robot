#include "client.h"
#include "pb_master.h"
#include "config.h" // Added for global_frame_header_config
#include <arpa/inet.h> // For htonl, htons (should be in common.h but good to ensure here)
#include <vector>      // For byte manipulation if needed
#include <iomanip>     // For std::setfill, std::setw (if using stringstream for hex)
#include <algorithm>   // For std::min

// Anonymous namespace for helper functions
namespace {

// Helper to append a string, padding with nulls or truncating to target_size
void append_string_fixed_size(std::string& dest, const std::string& src, int target_size) {
    if (target_size <= 0) return;
    std::string temp = src;
    if (temp.length() > static_cast<size_t>(target_size)) {
        temp.resize(target_size);
    } else {
        temp.resize(target_size, '\0'); // Pad with null characters
    }
    dest.append(temp);
}

// --- Integer to Byte Array Helpers ---
// Note: These assume network byte order is Big Endian for BE types.
// For LE types, direct memory copy is fine on LE machines. For portability, byte-by-byte construction is better.

void append_uint8(std::string& dest, uint8_t val) {
    dest.push_back(static_cast<char>(val));
}

void append_int8(std::string& dest, int8_t val) {
    dest.push_back(static_cast<char>(val));
}

void append_uint16_be(std::string& dest, uint16_t val) {
    uint16_t net_val = htons(val);
    dest.append(reinterpret_cast<const char*>(&net_val), sizeof(net_val));
}

void append_uint16_le(std::string& dest, uint16_t val) {
    char bytes[2];
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
    dest.append(bytes, 2);
}

void append_int16_be(std::string& dest, int16_t val) {
    uint16_t net_val = htons(static_cast<uint16_t>(val));
    dest.append(reinterpret_cast<const char*>(&net_val), sizeof(net_val));
}

void append_int16_le(std::string& dest, int16_t val) {
    char bytes[2];
    uint16_t u_val = static_cast<uint16_t>(val);
    bytes[0] = u_val & 0xFF;
    bytes[1] = (u_val >> 8) & 0xFF;
    dest.append(bytes, 2);
}

void append_uint32_be(std::string& dest, uint32_t val) {
    uint32_t net_val = htonl(val);
    dest.append(reinterpret_cast<const char*>(&net_val), sizeof(net_val));
}

void append_uint32_le(std::string& dest, uint32_t val) {
    char bytes[4];
    bytes[0] = val & 0xFF;
    bytes[1] = (val >> 8) & 0xFF;
    bytes[2] = (val >> 16) & 0xFF;
    bytes[3] = (val >> 24) & 0xFF;
    dest.append(bytes, 4);
}

void append_int32_be(std::string& dest, int32_t val) {
    uint32_t net_val = htonl(static_cast<uint32_t>(val));
    dest.append(reinterpret_cast<const char*>(&net_val), sizeof(net_val));
}

void append_int32_le(std::string& dest, int32_t val) {
    char bytes[4];
    uint32_t u_val = static_cast<uint32_t>(val);
    bytes[0] = u_val & 0xFF;
    bytes[1] = (u_val >> 8) & 0xFF;
    bytes[2] = (u_val >> 16) & 0xFF;
    bytes[3] = (u_val >> 24) & 0xFF;
    dest.append(bytes, 4);
}

// For 64-bit, htobe64/le64toh are preferred if available (POSIX 2008, but check system)
// Otherwise, manual byte swapping.
void append_uint64_be(std::string& dest, uint64_t val) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint64_t net_val = __builtin_bswap64(val);
#else
    uint64_t net_val = val;
#endif
    dest.append(reinterpret_cast<const char*>(&net_val), sizeof(net_val));
}

void append_uint64_le(std::string& dest, uint64_t val) {
#if __BYTE_ORDER == __BIG_ENDIAN
    uint64_t net_val = __builtin_bswap64(val);
#else
    uint64_t net_val = val;
#endif
    dest.append(reinterpret_cast<const char*>(&net_val), sizeof(net_val));
}

void append_int64_be(std::string& dest, int64_t val) {
    append_uint64_be(dest, static_cast<uint64_t>(val));
}

void append_int64_le(std::string& dest, int64_t val) {
    append_uint64_le(dest, static_cast<uint64_t>(val));
}

} // end anonymous namespace

using google::protobuf::Reflection;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;

const int kBlockSize = 4096;


Client::~Client() {
	if (is_connected()) {
		close_connection();
	}
	if (buffer_.recvbuf) {
		free(buffer_.recvbuf);
		buffer_.recvbuf = 0;
	}
	if (buffer_.sendbuf) {
		free(buffer_.sendbuf);
		buffer_.sendbuf = 0;
	}
	buffer_.recvlen = buffer_.sendlen = buffer_.recvbuf_len = buffer_.sendbuf_len = 0;
}

Client::Client(int max_pkg_len, bool has_checksum)
	: connfd_(-1),
	  max_pkg_len_(max_pkg_len),
	  has_checksum_(has_checksum) { }

bool Client::send_msg(const Message &msghead, const Message &msg, std::ostringstream &err) {
	if (!is_connected()) {
		err << "Not connecting to peer: " << peer_addr_
			<< ", when sending msg: " << msg.GetTypeName();
		return false;
	}

	std::string pkg;
	if (!encode(msghead, msg, pkg)) {
		err << "failed send: err encode: " << msg.GetTypeName();
		return false;
	}
	if (static_cast<int32_t>(pkg.size()) > max_pkg_len_) {
		err << "failed send: too big msg: " << msg.GetTypeName()
			<< ", size=" << pkg.size() << " > max_pkg_len=" << max_pkg_len_;
		return false;
	}

	if (buffer_.sendlen + (int)pkg.size() + 128 >= buffer_.sendbuf_len) { // 快满了, 扩容
		int new_size = calc_buffer_size(buffer_.sendbuf_len + pkg.size());
		buffer_.sendbuf = (char *)realloc(buffer_.sendbuf, new_size);
		buffer_.sendbuf_len = new_size;
	}
	memcpy(buffer_.sendbuf + buffer_.sendlen, pkg.c_str(), pkg.size());
	buffer_.sendlen += pkg.size();

	return true;
}

bool Client::recv_msg(Message **msghead, Message **msg, bool &complete, std::ostringstream &err) {
	complete = false;
	if (buffer_.recvlen < 4) {
		return true;
	}
	char *recvbuf = buffer_.recvbuf;
	int len = ntohl(*(uint32_t *)recvbuf);
	if (len < 8 || len > max_pkg_len_) {
		err << "failed recv: invalid msg,"
			<< " errlen=" << len << ", not in range [8, " << max_pkg_len_ << "]";
		return false;
	}
	if (buffer_.recvlen < len) {
		return true;
	}
	std::string recvstr(recvbuf, len);
	if (!decode(recvstr, msghead, msg)) {
		err << "failed recv: err decode msg";
		return false;
	}
	if (buffer_.recvlen == len) {
		buffer_.recvlen = 0;
	} else {
		memmove(buffer_.recvbuf, buffer_.recvbuf + len, buffer_.recvlen - len);
		buffer_.recvlen -= len;
	}
	complete = true;
	return true;
}

bool Client::encode(const Message &msghead, const Message &msg, std::string &pkg) {
    pkg.clear();
    std::string type_name = msg.GetTypeName(); // Used for logging

    if (global_frame_header_config_loaded) {
        // Custom frame header logic
        std::string s_msghead_pb;
        if (!msghead.SerializeToString(&s_msghead_pb)) {
            LOG(ERROR) << "Failed to serialize msghead for: " << type_name;
            return false;
        }
        std::string s_msgbody_pb;
        if (!msg.SerializeToString(&s_msgbody_pb)) {
            LOG(ERROR) << "Failed to serialize msg body for: " << type_name;
            return false;
        }

        size_t csmsghead_pb_len = s_msghead_pb.length();
        size_t csmsgbody_pb_len = s_msgbody_pb.length();
        size_t total_payload_len = csmsghead_pb_len + csmsgbody_pb_len;

        // Pre-calculate total size of all custom frame fields for CALC_TOTAL_PACKET_LENGTH
        size_t total_custom_header_fields_size = 0;
        for (const auto& field_def : global_frame_header_config.fields) {
            total_custom_header_fields_size += field_def.size_bytes;
        }

        for (const auto& field_def : global_frame_header_config.fields) {
            uint64_t value_to_pack_numeric = 0;
            std::string value_to_pack_string;
            bool is_numeric_value = false;

            switch (field_def.value_rule) {
                case FrameFieldValueRule::LITERAL:
                    if (std::holds_alternative<std::int64_t>(field_def.literal_value)) {
                        value_to_pack_numeric = static_cast<uint64_t>(std::get<std::int64_t>(field_def.literal_value));
                        is_numeric_value = true;
                    } else if (std::holds_alternative<std::string>(field_def.literal_value)) {
                        value_to_pack_string = std::get<std::string>(field_def.literal_value);
                    } else {
                        LOG(ERROR) << "Field " << field_def.name << ": Literal value is not set or has unexpected variant type.";
                        return false;
                    }
                    break;
                case FrameFieldValueRule::CALC_TOTAL_PACKET_LENGTH:
                    // Value is total size of custom header + total protobuf payload size
                    value_to_pack_numeric = total_custom_header_fields_size + total_payload_len;
                    is_numeric_value = true;
                    break;
                case FrameFieldValueRule::CALC_PROTOBUF_HEAD_LENGTH:
                    // Value is size of this specific field + CsMsgHead protobuf length
                    // This interpretation might need refinement based on exact requirements,
                    // e.g., if it's the length of CsMsgHead only, or CsMsgHead + some fixed offset.
                    // The prompt said "X + CsMsgHead.ByteSize()", where X is a configured constant.
                    // Assuming for now field_def.size_bytes IS X, or it means the field *contains* X + head_len.
                    // Let's assume it's the length of the CsMsgHead itself for now.
                    // A common interpretation is that this field *stores* the length of the protobuf head.
                    value_to_pack_numeric = csmsghead_pb_len; 
                    // If it should include its own size or other parts, adjust here.
                    // E.g., if the rule meant "size of this field + csmsghead_pb_len", it might be:
                    // value_to_pack_numeric = field_def.size_bytes + csmsghead_pb_len; 
                    // This needs clarification from requirements. For now, using csmsghead_pb_len.
                    is_numeric_value = true;
                    break;
                default:
                    LOG(ERROR) << "Field " << field_def.name << ": Unknown or unsupported value_rule.";
                    return false;
            }

            // Pack the value
            switch (field_def.data_type) {
                case FrameFieldDataType::UINT8:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for UINT8 field " << field_def.name; return false; }
                    append_uint8(pkg, static_cast<uint8_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::INT8:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for INT8 field " << field_def.name; return false; }
                    append_int8(pkg, static_cast<int8_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::UINT16_LE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for UINT16_LE field " << field_def.name; return false; }
                    append_uint16_le(pkg, static_cast<uint16_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::UINT16_BE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for UINT16_BE field " << field_def.name; return false; }
                    append_uint16_be(pkg, static_cast<uint16_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::INT16_LE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for INT16_LE field " << field_def.name; return false; }
                    append_int16_le(pkg, static_cast<int16_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::INT16_BE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for INT16_BE field " << field_def.name; return false; }
                    append_int16_be(pkg, static_cast<int16_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::UINT32_LE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for UINT32_LE field " << field_def.name; return false; }
                    append_uint32_le(pkg, static_cast<uint32_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::UINT32_BE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for UINT32_BE field " << field_def.name; return false; }
                    append_uint32_be(pkg, static_cast<uint32_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::INT32_LE:
                     if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for INT32_LE field " << field_def.name; return false; }
                    append_int32_le(pkg, static_cast<int32_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::INT32_BE:
                     if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for INT32_BE field " << field_def.name; return false; }
                    append_int32_be(pkg, static_cast<int32_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::UINT64_LE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for UINT64_LE field " << field_def.name; return false; }
                    append_uint64_le(pkg, value_to_pack_numeric);
                    break;
                case FrameFieldDataType::UINT64_BE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for UINT64_BE field " << field_def.name; return false; }
                    append_uint64_be(pkg, value_to_pack_numeric);
                    break;
                case FrameFieldDataType::INT64_LE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for INT64_LE field " << field_def.name; return false; }
                    append_int64_le(pkg, static_cast<int64_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::INT64_BE:
                    if (!is_numeric_value) { LOG(ERROR) << "Numeric value expected for INT64_BE field " << field_def.name; return false; }
                    append_int64_be(pkg, static_cast<int64_t>(value_to_pack_numeric));
                    break;
                case FrameFieldDataType::FIXED_STRING:
                    if (is_numeric_value) { LOG(ERROR) << "String value expected for FIXED_STRING field " << field_def.name; return false; }
                    append_string_fixed_size(pkg, value_to_pack_string, field_def.size_bytes);
                    break;
                default:
                    LOG(FATAL) << "Field " << field_def.name << ": Unimplemented data type for packing: " << static_cast<int>(field_def.data_type);
                    return false; // Should be unreachable due to FATAL
            }
        }
        // Append serialized protobuf messages
        pkg.append(s_msghead_pb);
        pkg.append(s_msgbody_pb);

    } else {
        // Original logic
        int32_t csmsghead_pb_len = msghead.ByteSize(); // Original: msghead.ByteSize()
        int32_t csmsgbody_pb_len = msg.ByteSize(); // Original: msg.ByteSize()
        
        // These lengths are for the original hardcoded header structure
        int32_t fixed_head_field_len = 4; // for the headlen field itself
        int32_t headlen_val_payload = fixed_head_field_len + csmsghead_pb_len;
        int32_t headlen_val_network = htonl(headlen_val_payload);

        // totlen includes its own 4 bytes, the headlen field's 4 bytes, 
        // the protobuf head, and the protobuf body. Checksum added later.
        int32_t totlen_val_payload = fixed_head_field_len + fixed_head_field_len + csmsghead_pb_len + csmsgbody_pb_len;
        int32_t totlen_val_network = htonl(totlen_val_payload + (has_checksum_ ? 4 : 0) );


        pkg.append(reinterpret_cast<const char*>(&totlen_val_network), sizeof(totlen_val_network));
        pkg.append(reinterpret_cast<const char*>(&headlen_val_network), sizeof(headlen_val_network));

        if (!msghead.AppendToString(&pkg)) {
            LOG(ERROR) << "Failed encode msghead for: " << type_name;
            return false;
        }
        if (!msg.AppendToString(&pkg)) {
            LOG(ERROR) << "Failed encode msg for: " << type_name;
            return false;
        }
    }

    // Checksum logic (applied to the fully constructed pkg)
    if (has_checksum_) {
        uint32_t sum = calc_checksum(pkg.c_str(), 0, pkg.size());
        sum = htonl(sum);
        pkg.append(reinterpret_cast<const char*>(&sum), sizeof(sum));
    }

    // VLOG(2) from original, can be adapted
    VLOG(2) << "[ENCODE:" << pkg.size()
            << ": (msghead_pb_len=" << msghead.ByteSize() 
            << ", msgbody_pb_len=" << msg.ByteSize() << ")]\n"
            << msghead.Utf8DebugString() << msg.Utf8DebugString();
    return true;
}

bool Client::decode(const std::string &pkg, Message **msghead, Message **msg) {
	*msghead = 0;
	*msg = 0;

	int min_totlen = 8; // sizeof(totlen) + sizeof(headlen)
	min_totlen += has_checksum_ ? 4 : 0; // [sizeof(checksum)]

	int getlen = static_cast<int>(pkg.size());
	if (getlen < min_totlen) {
		LOG(ERROR) << "decode err: getlen(" << getlen << ") < min(" << min_totlen << ")";
		return false;
	}
	// 至此, pkg 至少有 min_totlen 那么长 (保证解包不会越界)

	int32_t tlen_inpkg = ntohl(*(int32_t *)(pkg.c_str()));
	int32_t hlen_inpkg = ntohl(*(int32_t *)(pkg.c_str()+4));

	if (tlen_inpkg != getlen) {
		LOG(ERROR) << "decode err: getlen(" << getlen << ") != tlen_inpkg(" << tlen_inpkg << ")"; 
		return false;
	}

	if (hlen_inpkg < 4) {
		LOG(ERROR) << "decode err: hlen_inpkg(" << hlen_inpkg << ") < 4";
		return false;
	}

	// 头部最大长度:
	// 有校验和 = 全包长 - 包长字段 - 校验和字段 = tlen_inpkg - 4 - 4
	// 没有校验和 = 全包长 - 包长字段 = tlen_inpkg - 4
	int32_t max_headlen = has_checksum_ ? (tlen_inpkg-8) : (tlen_inpkg-4);

	if (hlen_inpkg > max_headlen) {
		LOG(ERROR) << "decode err: hlen_inpkg(" << hlen_inpkg << ") > max(" << max_headlen << ")";
		return false;
	}

#if 0
	// 检查校验和
	if (has_checksum_) {
		uint32_t sum_inpkg = ntohl(*(uint32_t *)(pkg.c_str()+tlen_inpkg-4));
		uint32_t sum = checksum(pkg.c_str(), 0, tlen_inpkg-4);
		if (sum_inpkg != sum) {
			LOG(ERROR) << "decode err: sum_inpkg(" << sum_inpkg << ") != sum(" << sum << ")";
			return false;
		}
	}
#endif

	// 解析包头部分
	std::string head_type_name = "ISeer20CSProto.cs_msg_head_t";
	std::string type_name_field_name = "msg_type_name";
	*msghead = PB_MASTER.create_message(head_type_name);
	if (!(*msghead)->ParseFromArray(pkg.c_str()+8, hlen_inpkg-4)) {
		LOG(ERROR) << "decode err: failed parse msghead";
		delete *msghead;
		*msghead = 0;
		return false;
	}

	const FieldDescriptor *type_name_field_descriptor
		= PB_MASTER.get_message_field_descriptor(*msghead, type_name_field_name);
	if (!type_name_field_descriptor) {
		LOG(ERROR) << "decode err: msghead has no type_name field with name: "
			<< type_name_field_name;
		delete *msghead;
		*msghead = 0;
		return false;
	}
	const Reflection *type_name_field_reflection = (*msghead)->GetReflection();
	const std::string &type_name
		= type_name_field_reflection->GetString(**msghead, type_name_field_descriptor);

	*msg = PB_MASTER.create_message(type_name);
	if (!(*msg)) {
		LOG(ERROR) << "decode err: failed create_message: " << type_name;
		delete *msghead;
		*msghead = 0;
		return false;
	}

	const char *data = pkg.c_str() + 4 + hlen_inpkg;
	int32_t datalen = tlen_inpkg - 4 - hlen_inpkg;
	if (has_checksum_) {
		datalen -= 4;
	}

	if (!(*msg)->ParseFromArray(data, datalen)) {
		LOG(ERROR) << "decode err: failed parse message: " << type_name;
		delete *msghead;
		*msghead = 0;
		delete *msg;
		*msg = 0;
		return false;
	}

	VLOG(2) << "[DECODE]\n"
		<< (*msghead)->Utf8DebugString() << (*msg)->Utf8DebugString();
	return true;
}

int Client::parse_sockaddr(const char *str, struct sockaddr *out, int *outlen) {
	int port;
	char buf[128];
	const char *cp, *addr_part, *port_part;
	int is_ipv6;
	/* recognized formats are:
	 * [ipv6]:port
	 * ipv6
	 * [ipv6]
	 * ipv4:port
	 * ipv4
	 */

	cp = strchr(str, ':');
	if (*str == '[') {
		int len;
		if (!(cp = strchr(str, ']'))) {
			return -1;
		}
		len = (int) ( cp-(str + 1) );
		if (len > (int)sizeof(buf)-1) {
			return -1;
		}
		memcpy(buf, str+1, len);
		buf[len] = '\0';
		addr_part = buf;
		if (cp[1] == ':')
			port_part = cp+2;
		else
			port_part = NULL;
		is_ipv6 = 1;
	} else if (cp && strchr(cp+1, ':')) {
		is_ipv6 = 1;
		addr_part = str;
		port_part = NULL;
	} else if (cp) {
		is_ipv6 = 0;
		if (cp - str > (int)sizeof(buf)-1) {
			return -1;
		}
		memcpy(buf, str, cp-str);
		buf[cp-str] = '\0';
		addr_part = buf;
		port_part = cp+1;
	} else {
		addr_part = str;
		port_part = NULL;
		is_ipv6 = 0;
	}

	if (port_part == NULL) {
		port = 0;
	} else {
		port = atoi(port_part);
		if (port <= 0 || port > 65535) {
			return -1;
		}
	}

	if (!addr_part) {
		return -1; /* Should be impossible. */
	}

	if (is_ipv6) {
		struct sockaddr_in6 sin6;
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(port);
		if (inet_pton(AF_INET6, addr_part, &sin6.sin6_addr) != 1) {
			return -1;
		}
		if ((int)sizeof(sin6) > *outlen) {
			return -1;
		}
		memset(out, 0, *outlen);
		memcpy(out, &sin6, sizeof(sin6));
		*outlen = sizeof(sin6);
		return 0;
	}

	// ipv4
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	if (inet_pton(AF_INET, addr_part, &sin.sin_addr) != 1) {
		return -1;
	}
	if ((int)sizeof(sin) > *outlen) {
		return -1;
	}
	memset(out, 0, *outlen);
	memcpy(out, &sin, sizeof(sin));
	*outlen = sizeof(sin);
	return 0;
}

int Client::tcp_connect(const std::string &svraddr, std::ostringstream &err) {
	struct sockaddr_in peer;
	int peer_addrlen = sizeof(peer);
	memset(&peer, 0, sizeof(peer));
	if (parse_sockaddr(svraddr.c_str(), (struct sockaddr *)&peer, &peer_addrlen) == -1) {
		err << "invalid peeraddr: " << svraddr;
		return -1;
	}

	int s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		err << "socket: " << strerror(errno);
		return -1;
	}
	if (connect(s, (const sockaddr*)&peer, sizeof(peer)) == -1) {
		err << "connect: " << strerror(errno);
		close(s);
		return -1;
	}
	if (set_fd_nonblock(s) == -1) {
		err << "set_fd_nonblock: " << strerror(errno);
		close(s);
		return -1;
	}
	if (set_tcp_nodelay(s) == -1) {
		err << "set_tcp_nondelay: " << strerror(errno);
		close(s);
		return -1;
	}

	return s;
}

int Client::net_tcp_recv(std::ostringstream &errmsg) {
	int nread = 0;
    while(true) {
        nread = read(connfd_, buffer_.recvbuf + buffer_.recvlen,
				buffer_.recvbuf_len - buffer_.recvlen);
        if (nread == 0) { // EOF
			errmsg << "recv meet EOF (peer shutdown), fd: " << connfd_;
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
        if (nread == -1) {
			if (errno == EINTR) { continue; }
			if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
			// other err
			errmsg << "recv meet error, fd: " << connfd_
				<< ", err(" << errno << "): " << strerror(errno);
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
        buffer_.recvlen += nread;
		if (buffer_.recvlen + 128 >= buffer_.recvbuf_len) { // 满了, 扩容
			int new_size = calc_buffer_size(buffer_.recvbuf_len + kBlockSize);
			buffer_.recvbuf = (char *)realloc(buffer_.recvbuf, new_size);
			buffer_.recvbuf_len = new_size;
		}
    }
	return 0;
}

int Client::net_tcp_send(std::ostringstream &errmsg) {
	if (buffer_.sendlen == 0) return 0;
	int total_sent = 0;
	int nwritten = 0;
	while(true) {
		nwritten = write(connfd_, buffer_.sendbuf + total_sent, buffer_.sendlen - total_sent);
		if (nwritten == 0) { // EOF
			errmsg << "send meet EOF??? (peer shutdown), fd: " << connfd_;
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
		if (nwritten == -1) {
			if (errno == EINTR) { continue; }
			if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
			errmsg << "send meet error, fd: " << connfd_
				<< ", err(" << errno << "): " << strerror(errno);
			close(connfd_);
			connfd_ = -1;
			return -1;
		}
		total_sent += nwritten;
		if (total_sent == buffer_.sendlen) { break; }
	}
	if (total_sent == buffer_.sendlen) {
		buffer_.sendlen = 0;
	} else { // total_sent < buffer_.sendlen
		memmove(buffer_.sendbuf, buffer_.sendbuf + total_sent, buffer_.sendlen - total_sent);
		buffer_.sendlen -= total_sent;
	}
	return 0;
}
