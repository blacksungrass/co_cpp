#pragma once
#include "co_env.h"
#include "connection.h"
#include "socks5_client.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <set>

class socks5_client;

class socks5_server{
private:
    int m_port;
    int m_listen_socket;
    co_env* m_co_env;
    int m_listen_cothread;
    sockaddr_in m_server_addr;
    std::set<socks5_client*> m_clients;
public:
    socks5_server(int port);
    void run();
    static void listen_func(co_thread&,void*);
    void close_client(socks5_client*);
    ~socks5_server();
};