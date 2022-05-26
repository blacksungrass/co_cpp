#pragma once

#include "co_env.h"
#include "connection.h"
#include "socks5_server.h"
#include <netinet/in.h>

class socks5_server;

class socks5_client{
private:
    co_env* m_co_env;
    socks5_server* const m_server;
    int m_socket,m_remote_socket;
    sockaddr_in m_remote_addr;
    sockaddr_in* m_server_addr;
    ushort m_server_port;
    int m_cothread;

    bool handshake(co_thread&);
    bool handle_request(co_thread&);
public:
    socks5_client(int socket,socks5_server* server,co_env* env,sockaddr_in*);
    static void process(co_thread&,void*);
    
    void close();
    ~socks5_client();
};