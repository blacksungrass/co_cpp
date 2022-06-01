#include "socks5_client.h"
#include <fcntl.h>
#include <cstring>

static int make_non_blocking(int fd){
    int oldflag = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,oldflag|O_NONBLOCK);
    return oldflag;
}

static void copy(char* buf,uint32_t val){
    buf[0] = (val)&0xff;
    buf[1] = (val&(0xff00))>>8;
    buf[2] = (val&(0xff0000))>>16;
    buf[3] = (val&(0xff000000))>>24;
}

static void copy(char* buf,uint16_t val){
    buf[0] = val&0xff;
    buf[1] = (val&(0xff00))>>8;
}

static void copy_to(char* buf,uint32_t& val){
    val = 0;
    val |= ((unsigned char)buf[3]<<24);
    val |= ((unsigned char)buf[2]<<16);
    val |= ((unsigned char)buf[1]<<8);
    val |= ((unsigned char)buf[0]);
}

static void copy_to(char* buf,uint16_t& val){
    val = 0;
    val |= ((unsigned char)buf[1]<<8);
    val |= ((unsigned char)buf[0]);
}

socks5_client::socks5_client(int socket,socks5_server* server,co_env* env,sockaddr_in* server_addr)
    :m_server(server),m_co_env(env),m_socket(socket)
{
    make_non_blocking(m_socket);
    m_server_addr = server_addr;
    m_server_port = ntohs(m_server_addr->sin_port);
    m_cothread = m_co_env->add_task(process,this);
    m_remote_socket = -1;
    m_trans_cothread1 = m_trans_cothread2 = -1;
}

void socks5_client::process(co_thread& thread,void* args){
    socks5_client* self = (socks5_client*)(args);
    connection conn(self->m_socket,thread);
    if(!self->handshake(thread)){
        self->close();
        return;
    }
    if(!self->handle_request(thread)){
        self->close();
        return;
    }
    self->m_trans_cothread1 = self->m_co_env->add_task(trans_func1,self);
    self->m_trans_cothread2 = self->m_co_env->add_task(trans_func2,self);
    self->m_cothread = -1;

    return;
}

bool socks5_client::handle_request(co_thread& thread){
    connection conn(m_socket,thread);
    char buf[300];
    char retbuf[50] = {5,0,0,1};
    copy(retbuf+4,m_server_addr->sin_addr.s_addr);
    copy(retbuf+8,m_server_addr->sin_port);

    int r = conn.read_n_bytes(4,buf);
    if(r<0)
        return false;
    if(buf[0]!=5){
        retbuf[1] = 0xff;
        conn.write_some(retbuf,10);
        return false;
    }
    if(buf[1]!=0x01){//if command is not cnnect
        retbuf[1] = 0x07;//unsupported command
        conn.write_some(retbuf,10);
        return false;
    }
    if(buf[3]!=0x01){//if addr is not ipv4
        retbuf[1] = 0x08;//unsupported addr type
        conn.write_some(retbuf,10);
        return false;
    }
    r = conn.read_n_bytes(6,buf+4);
    if(r<0)
        return false;
    
    memset(&m_remote_addr,0,sizeof(m_remote_addr));
    copy_to(buf+4,m_remote_addr.sin_addr.s_addr);
    copy_to(buf+8,m_remote_addr.sin_port);
    
    m_remote_addr.sin_family = AF_INET;
    m_remote_socket = socket(AF_INET,SOCK_STREAM,0);
    make_non_blocking(m_remote_socket);
    connection remote_conn(m_remote_socket,thread);
    bool ret = remote_conn.connect((sockaddr*)&m_remote_addr,sizeof(m_remote_addr));
    
    if(!ret){
        retbuf[1] = 0x01;
        conn.write_some(retbuf,10);
        return false;
    }
    retbuf[1] = 0x00;
    conn.write_some(retbuf,10);
    return true;
}

bool socks5_client::handshake(co_thread& thread){
    connection conn(m_socket,thread);
    char buf[300];
    int r = conn.read_n_bytes(1,buf);
    if(r<0){
        return false;
    }
    if(buf[0]!=5){
        return false;
    }
    int method_cnt = 0;
    r = conn.read_n_bytes(1,buf);
    if(r<0){
        return false;
    }
    method_cnt = (unsigned char)(buf[0]);
    if(method_cnt==0){
        return false;
    }
    r = conn.read_n_bytes(method_cnt,buf);
    if(r<0){
        return false;
    }
    bool have_no_auth = false;
    for(int i=0;i<method_cnt;++i){
        if(buf[i]==0){
            have_no_auth = true;
            break;
        }
    }
    if(!have_no_auth){
        buf[0] = 5;
        buf[1] = 0xff;
        conn.write_some(buf,2);
        return false;
    }
    buf[0] = 5;
    buf[1] = 0;
    conn.write_some(buf,2);
    return true;
}

void socks5_client::close(){
    m_server->close_client(this);
}

socks5_client::~socks5_client(){
    ::close(m_socket);
    if(m_remote_socket>=0){
        ::close(m_remote_socket);
    }
    if(m_cothread>=0){
        m_co_env->cancel_task(m_cothread);
    }
    if(m_trans_cothread1>=0){
        m_co_env->cancel_task(m_trans_cothread1);
    }
    if(m_trans_cothread2>=0){
        m_co_env->cancel_task(m_trans_cothread2);
    }
}

void socks5_client::trans_func1(co_thread& thread,void* args){
    socks5_client* self = (socks5_client*)(args);
    connection conn1(self->m_socket,thread);
    connection conn2(self->m_remote_socket,thread);
    char buf[3*1024];
    while(true){
        int r = conn1.read_some(buf,3*1024);
        if(r<0){
            self->close();
            return;
        }
        r = conn2.write_some(buf,r);
        if(r<0){
            self->close();
            return;
        }
    }
}

void socks5_client::trans_func2(co_thread& thread,void* args){
    socks5_client* self = (socks5_client*)(args);
    connection conn2(self->m_socket,thread);
    connection conn1(self->m_remote_socket,thread);
    char buf[3*1024];
    while(true){
        int r = conn1.read_some(buf,3*1024);
        if(r<0){
            self->close();
            return;
        }
        r = conn2.write_some(buf,r);
        if(r<0){
            self->close();
            return;
        }
    }
}