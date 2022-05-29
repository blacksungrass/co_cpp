#include "connection.h"


connection::connection(int socket,co_thread& t):m_fd(socket),m_thread(t){
    m_old_sighandler = signal(SIGPIPE,SIG_IGN);
}

//todo add timeout parameter
int connection::read_n_bytes(int n, char* buf, int timeout){
    int ret = n;
    while(n>0){
        int bytes_read = read(m_fd,buf,n);
        if(bytes_read==0){
            //eof
            //m_thread.suicide();
            return -1;
        }
        else if(bytes_read==n){
            break;
        }
        else if(bytes_read<0){
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                m_thread.yield_event(m_fd,EPOLLIN);
            }
            else{
                //got error
                //m_thread.suicide();
                return -1;
            }
        }
        else{
            buf += bytes_read;
            n -= bytes_read;
        }
    }
    return ret;
}

//todo add timeout parameter
int connection::read_some(char* buf, int maxnum, int timeout){
    int bytes_read = recv(m_fd,buf,maxnum,MSG_DONTWAIT);
    if(bytes_read<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK || bytes_read==0){
        //got error or eof
        //m_thread.suicide();
        return -1;
    }
    if(bytes_read<0){
        m_thread.yield_event(m_fd,EPOLLIN);
        bytes_read = read(m_fd,buf,maxnum);
        if(bytes_read<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK || bytes_read==0){
            return -1;
        }
    }
    return bytes_read;
    
}

//todo add timeout parameter
int connection::write(char* buf, int n, int timeout){
    int ret = n;
    while(n>0){
        auto sent = ::write(m_fd,buf,n);
        if(sent==n)
            break;
        else if(sent==0){
            //eof
            //m_thread.suicide();
            return -1;
        }
        else if(sent<0){
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                m_thread.yield_event(m_fd,EPOLLOUT);
            }
            else{
                //got error
                //m_thread.suicide();
                return -1;
            }
        }
        else{
            n -= sent;
            buf += sent;
            //todo add m_thread.yield_event(m_fd,EPOLLOUT); here?
        }
    }
    return ret;
}

bool connection::connect(sockaddr* addr, socklen_t len, int timeout){
    int ret = ::connect(m_fd,addr,len);
    if(ret==0)
        return true;
    if(errno!=EINPROGRESS)
        return false;
    m_thread.yield_event(m_fd,EPOLLOUT);
    int val;
    socklen_t t;
    ret = getsockopt(m_fd,SOL_SOCKET,SO_ERROR,&val,&t);
    if(ret!=0)
        return false;
    return val==0;
}

connection::~connection(){
    signal(SIGPIPE,m_old_sighandler);
}