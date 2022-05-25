#include "co_env.h"
#include "connection.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/cdefs.h>
#include "log.h"

//forward declare
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

time_t get_time_now(){
    struct timeval tv;
    gettimeofday(&tv,nullptr);
    return tv.tv_sec;
}

int make_non_blocking(int fd){
    int oldflag = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,oldflag|O_NONBLOCK);
    return oldflag;
}

class echo_client{
private:
    time_t m_last_activate_time;
    int m_socket;
    co_env* m_env;
    echo_server* m_server;
    //bool m_closed;
    int m_task_id;
public:
    echo_client(int s,co_env* env,echo_server* server){
        m_socket = s;
        m_env = env;
        //m_closed = false;
        m_server = server;
        make_non_blocking(m_socket);
        m_task_id = m_env->add_task(work,this);
        m_last_activate_time = get_time_now();
    }
    int get_last_active_time(){
        return m_last_activate_time;
    }
    static void work(co_thread& t,void* args){    
        echo_client* self = (echo_client*)(args);
        connection conn(self->m_socket,t);
        char buf[200];
        while(true){
            int cnt = conn.read_as_more_as_possible(buf,200);
            if(cnt<0)
                break;
            self->m_last_activate_time = get_time_now();
            int ret = conn.write(buf,cnt);
            if(ret<0)
                break;
            self->m_last_activate_time = get_time_now();
        }
        //self->m_closed = true;
        Log("client close due to error");
        self->m_server->close_client(self);
    }
    ~echo_client(){
        m_env->cancel_task(m_task_id);
        close(m_socket);
    }
};

echo_server::echo_server(int port,int timeout){
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
    m_listen_task_id = m_env->add_task(&listen_func,this);
    m_check_active_task_id = m_env->add_task(&check_inavtive_func,this);
}

void echo_server::run(){
    Log("run echo server");
    m_env->loop();
}

void echo_server::close_client(echo_client* client){
    Log("close client");
    delete client;
    m_conns.erase(client);
}

echo_server::~echo_server(){
    auto it = m_conns.begin();
    while(it!=m_conns.end()){
        delete (*it);
        it = m_conns.erase(it);
    }
    m_env->cancel_task(m_check_active_task_id);
    m_env->cancel_task(m_listen_task_id);
    close(m_listen_socket);
    delete m_env;

}

void echo_server::check_inavtive_func(co_thread& t,void* args){
    Log("start check inavtive cothread");
    echo_server* self = (echo_server*)(args);
    while(true){
        Log("check inactive cothread will yield from some time");
        t.yield_for(self->m_timeout*1000);
        Log("check inactive cothread resumed");
        int now = get_time_now();
        auto it = self->m_conns.begin();
        while(it!=self->m_conns.end()){
            int client_time = (*it)->get_last_active_time();
            if(now-client_time>self->m_timeout){
                //timeout, close this client
                Log("timeout, close client");
                delete (*it);
                it = self->m_conns.erase(it);
            }
            else{
                ++it;
            }
        }
    }
}

void echo_server::listen_func(co_thread& t,void* args){
    Log("start listen cothread");
    echo_server* self = (echo_server*)(args);
    while(true){
        t.yield_event(self->m_listen_socket,EPOLLIN);
        Log("listen thread ready to accept a new client");
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(sockaddr_in);
        int client_socket = accept(self->m_listen_socket,(sockaddr*)&client_addr,&addr_len);
        self->m_conns.emplace(new echo_client(client_socket,self->m_env,self));
    }
}
