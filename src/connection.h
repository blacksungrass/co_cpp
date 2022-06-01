#pragma once
#include "co_env.h"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/signal.h>

class connection{
private:
    int m_fd;//need to be non-blocking
    co_thread& m_thread;
    sighandler_t m_old_sighandler;//save old sighandler ,reset signal to ignore SIGPIPE
public:
    connection(int socket, co_thread& t);
    
    int read_n_bytes(int n, char* buf, int timeout=0);

    int read_some(char* buf, int maxnum, int timeout=0);

    int write_some(char* buf, int n, int timeout=0);

    bool connect(sockaddr* addr, socklen_t len, int timeout=0);

    ~connection();
};