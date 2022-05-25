#include "co_env.h"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

class connection{
private:
    int m_socket;//need to be non-blocking
    co_thread& m_thread;
public:
    connection(int socket,co_thread& t):m_socket(socket),m_thread(t){

    }
    
    int read_n_bytes(int n,char* buf){
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
    
    int read_as_more_as_possible(char* buf,int maxnum){
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
    
    int write(char* buf,int n){
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
            }
        }
        return ret;
    }
};