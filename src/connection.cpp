#include "connection.h"


connection::connection(int socket,co_thread& t):m_socket(socket),m_thread(t){

}

//todo add timeout parameter
int connection::read_n_bytes(int n,char* buf){
    int ret = n;
    while(n>0){
        int bytes_read = recv(m_socket,buf,n,0);
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
                m_thread.yield_event(m_socket,EPOLLIN);
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
int connection::read_some(char* buf,int maxnum){
    int bytes_read = recv(m_socket,buf,maxnum,0);
    if(bytes_read<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK || bytes_read==0){
        //got error or eof
        //m_thread.suicide();
        return -1;
    }
    if(bytes_read<0){
        m_thread.yield_event(m_socket,EPOLLIN);
        bytes_read = recv(m_socket,buf,maxnum,0);
        if(bytes_read<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK || bytes_read==0){
            return -1;
        }
    }
    return bytes_read;
    
}

//todo add timeout parameter
int connection::write(char* buf,int n){
    int ret = n;
    while(n>0){
        auto sent = send(m_socket,buf,n,0);
        if(sent==n)
            break;
        else if(sent==0){
            //eof
            //m_thread.suicide();
            return -1;
        }
        else if(sent<0){
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                m_thread.yield_event(m_socket,EPOLLOUT);
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
            //todo add m_thread.yield_event(m_socket,EPOLLOUT); here?
        }
    }
    return ret;
}

bool connection::connect(sockaddr* addr,socklen_t len){
    int ret = ::connect(m_socket,addr,len);
    if(ret==0)
        return true;
    if(errno!=EINPROGRESS)
        return false;
    m_thread.yield_event(m_socket,EPOLLOUT);
    int val;
    socklen_t t;
    ret = getsockopt(m_socket,SOL_SOCKET,SO_ERROR,&val,&t);
    if(ret!=0)
        return false;
    return val==0;
}