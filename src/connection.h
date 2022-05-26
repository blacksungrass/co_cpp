#pragma once
#include "co_env.h"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

class connection{
private:
    int m_socket;//need to be non-blocking
    co_thread& m_thread;
public:
    connection(int socket,co_thread& t);
    
    int read_n_bytes(int n,char* buf);

    int read_some(char* buf,int maxnum);

    int write(char* buf,int n);

    bool connect(sockaddr* addr,socklen_t len);
};