#ifndef __CLIENT_H__
#define __CLIENT_H__


#include "common.h"

extern const int kBlockSize;
using google::protobuf::Message;


struct Buffer {
public:
	Buffer() : recvlen(0), sendlen(0) {
			recvbuf_len = kBlockSize;
			sendbuf_len = kBlockSize;
			recvbuf = (char *)malloc(kBlockSize);
			sendbuf = (char *)malloc(kBlockSize);
		}

	void Clear(void) {
		recvlen = sendlen = 0;
	}

public:
	int32_t recvlen;
	int32_t sendlen;
	int32_t recvbuf_len;
	int32_t sendbuf_len;
	char *recvbuf;
	char *sendbuf;
};

class Client {
public:
	~Client();
	Client(int max_pkg_len, bool has_checksum);

public:
	inline bool is_connected(void);
	inline bool try_connect_to_peer(const std::string &svraddr, std::ostringstream &err);
	inline void close_connection(void);
	const Buffer &buffer(void) { return buffer_; }
	void clear_buffer(void) { buffer_.Clear(); }

public:
	bool send_msg(const Message &msghead, const Message &msg, std::ostringstream &err);
	bool recv_msg(Message **msghead, Message **msg, bool &complete, std::ostringstream &err); 
	bool encode(const Message &msghead, const Message &msg, std::string &pkg); 
	bool decode(const std::string &pkg, Message **msghead, Message **msg); 
	int tcp_connect(const std::string &svraddr, std::ostringstream &err);
	int net_tcp_send(std::ostringstream &errmsg);
	int net_tcp_recv(std::ostringstream &errmsg);


private:
	int parse_sockaddr(const char *str, struct sockaddr *out, int *outlen);
	inline uint32_t calc_checksum(const char *buf, int start, int len); 
	inline int set_fd_nonblock(int s);
	inline int set_tcp_nodelay(int s);
	inline int calc_buffer_size(int needsize);


private:
	int connfd_;
	std::string peer_addr_;
	int32_t max_pkg_len_;
	bool has_checksum_;
	Buffer buffer_;
};



#include "client-inl.h"


#endif // __CLIENT_H__
