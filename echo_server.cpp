#include "co_env.h"
#include "connection.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <fcntl.h>

int make_non_blocking(int fd){
    int oldflag = fcntl(fd,F_GETFL);
    fcntl(fd,oldflag|O_NONBLOCK);
    return oldflag;
}

class echo_client{
private:
    int m_socket;
    co_env* m_env;
public:
    echo_client(int s,co_env* env){
        m_socket = s;
        m_env = env;
        make_non_blocking(m_socket);
        m_env->add_task(work,this);
    }
    static void work(co_thread& t,void* args){    
        echo_client* self = (echo_client*)(args);
        connection conn(self->m_socket,t);
        char buf[200];
        while(true){
            int cnt = conn.read_as_more_as_possible(buf,200);
            conn.write(buf,cnt);
        }
    }
};

class echo_server{
private:
    int m_port;
    int m_timeout;
    int m_listen_socket;
    co_env* m_env;
    std::set<echo_client*> m_conns;
public:
    echo_server(int port,int timeout){
        m_port = port;
        m_timeout = timeout;
        m_listen_socket = socket(AF_INET,SOCK_STREAM,0);
        struct sockaddr_in addr{0};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        bind(m_listen_socket,(sockaddr*)&addr,sizeof(addr));
        listen(m_listen_socket,10);

        m_env = new co_env();
        m_env->add_task(&listen_func,this);
    }
    void run(){
        m_env->loop();
    }
    static void listen_func(co_thread& t,void* args){
        echo_server* self = (echo_server*)(args);
        while(true){
            t.yield_event(self->m_listen_socket,EPOLLIN);
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(sockaddr_in);
            int client_socket = accept(self->m_listen_socket,(sockaddr*)&client_addr,&addr_len);
            self->m_conns.emplace(new echo_client(client_socket,self->m_env));
        }
    }
};