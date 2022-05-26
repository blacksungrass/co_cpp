#include "socks5_server.h"
#include <netinet/in.h>
#include <unistd.h>

socks5_server::socks5_server(int port){
    m_port = port;
    m_co_env = new co_env();
    m_listen_socket = socket(AF_INET,SOCK_STREAM,0);
    m_server_addr.sin_family = AF_INET;
    m_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_server_addr.sin_port = htons(port);
    bind(m_listen_socket,(sockaddr*)&m_server_addr,sizeof(m_server_addr));
    listen(m_listen_socket,10);
    socklen_t tmp;
    getsockname(m_listen_socket,(sockaddr*)(&m_server_addr),&tmp);
    m_listen_cothread = m_co_env->add_task(&listen_func,this);
}

socks5_server::~socks5_server(){

}

void socks5_server::run(){
    m_co_env->loop();
}

void socks5_server::listen_func(co_thread& t,void* args){
    socks5_server* self = (socks5_server*)args;
    while(true){
        t.yield_event(self->m_listen_socket,EPOLLIN);
        struct sockaddr_in client_addr;
        socklen_t addr_len = 0;
        int client_socket = accept(self->m_listen_socket,(sockaddr*)&client_addr,&addr_len);
        socks5_client* client = new socks5_client(client_socket,self,self->m_co_env,&(self->m_server_addr));
        self->m_clients.insert(client);
    }
}

void socks5_server::close_client(socks5_client* client){
    if(m_clients.count(client)){
        delete client;
        m_clients.erase(client);
    }
}