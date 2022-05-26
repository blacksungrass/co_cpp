#pragma once
#include "co_env.h"
#include "connection.h"
#include "log.h"

class echo_client;

class echo_server{
private:
    int m_port;
    int m_timeout;
    int m_listen_socket;
    co_env* m_env;
    std::set<echo_client*> m_conns;
    int m_listen_task_id;
    int m_check_active_task_id;
public:
    echo_server(int port,int timeout);
    void run();
    void close_client(echo_client* client);
    ~echo_server();
    static void check_inavtive_func(co_thread& t,void* args);
    static void listen_func(co_thread& t,void* args);
};

class echo_client{
private:
    time_t m_last_activate_time;
    int m_socket;
    co_env* m_env;
    echo_server* m_server;
    int m_task_id;
public:
    echo_client(int s,co_env* env,echo_server* server);
    int get_last_active_time();
    static void work(co_thread& t,void* args);
    ~echo_client();
};