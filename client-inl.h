#ifndef __CLIENT_INL_H__
#define __CLIENT_INL_H__


inline bool Client::is_connected(void) {
	return (connfd_ >= 0);
}

inline bool Client::try_connect_to_peer(const std::string &svraddr, std::ostringstream &err) {
	if (is_connected()) { return true; }
	connfd_ = tcp_connect(svraddr, err);
	return (connfd_ == -1) ? false : true;
}

inline void Client::close_connection(void) {
	if (!is_connected()) return ;
	close(connfd_);
	connfd_ = -1;
}

inline uint32_t Client::calc_checksum(const char *buf, int start, int len) {
	buf = 0; start = len = 0; return 0; // make gcc shut up
}

inline int Client::set_fd_nonblock(int s) {
	int flags;
	if ((flags = fcntl(s, F_GETFL)) == -1) {
		return -1;
	}
	if (fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1) {
		return -1;
	}
	return 0;
}

inline int Client::set_tcp_nodelay(int s) {
    int yes = 1;
    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) {
        return -1;
    }
    return 0;
}

inline int Client::calc_buffer_size(int needsize) {
	return ((needsize + kBlockSize - 1) / kBlockSize) * kBlockSize;
}


#endif // __CLIENT_INL_H__
