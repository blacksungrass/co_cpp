#include "connection.h"
#include "fcntl.h"
#include <sys/timerfd.h>
#include <exception>

static void set_non_blocking(int fd){
    int old = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,old|O_NONBLOCK);
}

static int create_timer_fd(int ms){
    if(ms<=0){
        throw std::runtime_error("time in timerfd should be positive");
    }
    int fd = timerfd_create(CLOCK_MONOTONIC,0);
    struct itimerspec value{0};
    value.it_value.tv_sec = ms/1000;
    ms %= 1000;
    value.it_value.tv_nsec = ms*1000*1000;
    timerfd_settime(fd,0,&value,nullptr);
    return fd;
}

connection::connection(int socket,co_thread& t):m_fd(socket),m_thread(t){
    set_non_blocking(m_fd);
    m_old_sighandler = signal(SIGPIPE,SIG_IGN);
}

int connection::read_n_bytes(int n, char* buf, int timeout){
    if(n<=0)
        return 0;

    int timer_fd = -1;
    long timer_event_id = -1;
    if(timeout>0){
        timer_fd = create_timer_fd(timeout);
        timer_event_id = m_thread.register_event(timer_fd,EPOLLIN);
    }

    int ret = n;

    while(n>0){
        int bytes_read = read(m_fd,buf,n);
        if(bytes_read==0){
            //eof
            if(timer_fd>=0)
                m_thread.cancel_event(timer_event_id);
            return -1;
        }
        else if(bytes_read<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
            //blocked
            long read_event_id = m_thread.register_event(m_fd,EPOLLIN);
            long event_and_fd = m_thread.yield(true);
            int event = event_and_fd>>32;
            int fd = event_and_fd&(0xffffffff);
            //printf("decode event_and_fd:%lld event:%d fd:%d\n",event_and_fd,event,fd);
            if(timer_fd>=0&&fd==timer_fd||event&EPOLLHUP||event&EPOLLERR){
                //out of time or fd error
                m_thread.cancel_event(read_event_id);
                return -1;
            }
        }
        else if(bytes_read<0){
            //error
            if(timer_fd>=0)
                m_thread.cancel_event(timer_event_id);
            return -1;
        }
        else{
            //read partly
            buf += bytes_read;
            n -= bytes_read;

            if(n<=0)
                break;

            long read_event_id = m_thread.register_event(m_fd,EPOLLIN);
            long event_and_fd = m_thread.yield(true);
            int event = event_and_fd>>32;
            int fd = event_and_fd&(0xffffffff);
            //printf("event_and_fd:%lld event:%d fd:%d\n",event_and_fd,event,fd);
            if(timer_fd>=0&&fd==timer_fd||event&EPOLLHUP||event&EPOLLERR){
                //out of time or fd error
                m_thread.cancel_event(read_event_id);
                return -1;
            }
        }
    }
    if(timer_fd>=0)
        m_thread.cancel_event(timer_event_id);
    return ret;
}

int connection::read_some(char* buf, int maxnum, int timeout){
    if(maxnum<=0){
        return 0;
    }
    int timer_fd = -1;
    long timer_event_id = -1;
    if(timeout>0){
        timer_fd = create_timer_fd(timeout);
    }
    int bytes_read = read(m_fd,buf,maxnum);
    if(bytes_read<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK || bytes_read==0){
        //got error or eof
        //m_thread.suicide();
        if(timer_fd>=0)
            m_thread.cancel_event(timer_event_id);
        return -1;
    }
    if(bytes_read<0){
        long read_event_id = m_thread.register_event(m_fd,EPOLLIN);
        long event_and_fd = m_thread.yield(true);
        int event = event_and_fd>>32;
        int fd = event_and_fd&(0xffffffff);
        if(timer_fd>=0&&fd==timer_fd||event&EPOLLHUP||event&EPOLLERR){
            //out of time or fd error
            m_thread.cancel_event(read_event_id);
            return -1;
        }
        bytes_read = read(m_fd,buf,maxnum);
        if(bytes_read<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK || bytes_read==0){
            if(timer_fd>=0)
                m_thread.cancel_event(timer_event_id);
            return -1;
        }
    }
    if(timer_fd>=0)
        m_thread.cancel_event(timer_event_id);
    return bytes_read;
    
}

int connection::write_some(char* buf, int n, int timeout){
    if(n<=0)
        return 0;
    int timer_fd = -1;
    long timer_event_id = -1;
    if(timeout>0){
        timer_fd = create_timer_fd(timeout);
        timer_event_id = m_thread.register_event(timer_fd,EPOLLIN);
    }

    int ret = n;
    while(n>0){
        auto sent = write(m_fd,buf,n);
        if(sent==0){
            //eof
            if(timer_fd>=0)
                m_thread.cancel_event(timer_event_id);
            return -1;
        }
        else if(sent<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
            //blocked
            long write_event_id = m_thread.register_event(m_fd,EPOLLOUT);
            long event_and_fd = m_thread.yield(true);
            int event = event_and_fd>>32;
            int fd = event_and_fd&(0xffffffff);
            if(timer_fd>=0&&timer_fd==fd||event&EPOLLHUP||event&EPOLLERR){
                //out of time or fd error
                m_thread.cancel_event(write_event_id);
                return -1;
            }
        }
        else if(sent<0){
            //error
            if(timer_fd>=0)
                m_thread.cancel_event(timer_event_id);
            return -1;            
        }
        else{
            //write some
            n -= sent;
            buf += sent;

            if(n<=0)
                break;

            long write_event_id = m_thread.register_event(m_fd,EPOLLOUT);
            long event_and_fd = m_thread.yield(true);
            int event = event_and_fd>>32;
            int fd = event_and_fd&(0xffffffff);
            if(timer_fd>=0&&timer_fd==fd||event&EPOLLHUP||event&EPOLLERR){
                //out of time or fd error
                m_thread.cancel_event(write_event_id);
                return -1;
            }
        }
    }
    if(timer_fd>=0)
        m_thread.cancel_event(timer_event_id);
    return ret;
}

bool connection::connect(sockaddr* addr, socklen_t len, int timeout){
    int ret = ::connect(m_fd,addr,len);
    if(ret==0)
        return true;
    if(errno!=EINPROGRESS)
        return false;
    int timer_fd = -1;
    long timer_event_id = -1;
    if(timeout>0){
        timer_fd = create_timer_fd(timeout);
        timer_event_id = m_thread.register_event(timer_fd,EPOLLIN);
    }
    int out_event_id = m_thread.register_event(m_fd,EPOLLOUT);
    long event_and_fd = m_thread.yield(true);
    int event = event_and_fd>>32;
    int fd = event_and_fd&(0xffffffff);
    if(timer_fd>=0&&fd==timer_fd||event&EPOLLHUP||event&EPOLLERR){
        //out of time or fd error
        m_thread.cancel_event(out_event_id);
        return false;
    }
    m_thread.cancel_event(timer_event_id);
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