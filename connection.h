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
    
    void read_n_bytes(int n,char* buf){
        while(n>0){
            int bytes_read = recv(m_socket,buf,n,0);
            if(bytes_read==0){
                //eof
                m_thread.suicide();
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
                    m_thread.suicide();
                }
            }
            else{
                buf += bytes_read;
                n -= bytes_read;
            }
        }
    }
    
    int read_as_more_as_possible(char* buf,int maxnum){
        int bytes_read = recv(m_socket,buf,maxnum,0);
        if(bytes_read<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK || bytes_read==0){
            //got error or eof
            m_thread.suicide();
        }
        return bytes_read;
    }
    
    void write(char* buf,int n){
        while(n>0){
            auto sent = send(m_socket,buf,n,0);
            if(sent==n)
                break;
            else if(sent==0){
                //eof
                m_thread.suicide();
            }
            else if(sent<0){
                if(errno==EAGAIN||errno==EWOULDBLOCK){
                    m_thread.yield_event(m_socket,EPOLLOUT);
                }
                else{
                    //got error
                    m_thread.suicide();
                }
            }
            else{
                n -= sent;
                buf += sent;
            }
        }
    }
};