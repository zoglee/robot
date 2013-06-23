#include "client.h"
#include "pb_master.h"



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
	std::string type_name = msg.GetTypeName(); 

	int32_t headlen = htonl(4 + msghead.ByteSize());
	int32_t totlen = 4 + 4 + msghead.ByteSize() + msg.ByteSize();
	if (has_checksum_) {
		totlen += 4;
	}
	totlen = htonl(totlen);

	pkg.clear();
	pkg.append((char*)(&totlen), sizeof(totlen));
	pkg.append((char*)(&headlen), sizeof(headlen));

	if (!msghead.AppendToString(&pkg)) {
		LOG(ERROR) << "Failed encode msghead for: " << type_name;
		return false;
	}

	if (!msg.AppendToString(&pkg)) {
		LOG(ERROR) << "Failed encode msg for: " << type_name;
		return false;
	}

	if (has_checksum_) {
		uint32_t sum = calc_checksum(pkg.c_str(), 0, pkg.size());
		sum = htonl(sum);
		pkg.append((char*)(&sum), sizeof(sum));
	}

	VLOG(2) << "[ENCODE:" << pkg.size()
		<< ": headsize=" << msghead.ByteSize() << "(hlen=" << ntohl(headlen) << ")"
		<< ", bodysize=" << msg.ByteSize() << "]\n"
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
