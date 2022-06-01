#include "co_thread.h"
#include "co_env.h"
#include <exception>
#include <cstdlib>

/*
* x86-64 calling convention refer https://aaronbloomfield.github.io/pdr/book/x86-64bit-ccc-chapter.pdf
* caller need to save r10,r11 and parameters(rdi,rsi,rdx,rcx,r8,r9)
*/


co_thread::co_thread(co_thread_func_t func,void* args,co_env* env_p){
    m_env = env_p;
    const long stack_size = 1024;//todo stack size could be a constructor parameter
    finished = false;
    
    m_addr = (long*)malloc(stack_size*sizeof(long));
    m_local_state = (long*)malloc(12*sizeof(void*));
    
    long ret_addr_idx = stack_size-8;
    if((long)(m_addr+ret_addr_idx)%16!=0){
        ret_addr_idx += 1;
    }
    
    m_addr[0] = (long)func;//rip
    m_addr[1] = (long)(m_addr+ret_addr_idx);//rsp
    m_addr[2] = (long)(m_addr+ret_addr_idx);//rbp
    m_addr[3] = (long)(this);//rdi(first param)
    m_addr[4] = (long)(args);//rsi(seocnd param)
    m_addr[ret_addr_idx+2] = (long)this;
    m_addr[ret_addr_idx+1] = (long)&finish;
    m_addr[ret_addr_idx] = (long)func;
}

int co_thread::run_impl(){
    if(this->finished){
        return 1;
    }
    return 0;
}

void co_thread::yield_impl(){

}

void co_thread::yield_event_impl(int fd,int event){
    if(m_env==nullptr){
        throw std::runtime_error("co_thread need a co_env to to yield for some event");
    }
    m_env->register_event(fd,event,this);
}

void co_thread::yield_events_impl(int fds[],int events[],int n){
    if(m_env==nullptr){
        throw std::runtime_error("co_thread need a co_env to to yield for some event");
    }
    for(int i=0;i<n;++i){
        m_env->register_event(fds[i],events[i],this);
    }
}

void co_thread::yield_for_impl(int ms){
    if(m_env==nullptr){
        throw std::runtime_error("co_thread need a co_env to to yield_for some time");
    }
    int fd = timerfd_create(CLOCK_MONOTONIC,0);
    struct itimerspec value{0};
    value.it_value.tv_sec = ms/1000;
    ms %= 1000;
    value.it_value.tv_nsec = ms*1000*1000;
    if(value.it_value.tv_sec==0&&value.it_value.tv_nsec==0){
        value.it_value.tv_nsec = 1;
    }
    timerfd_settime(fd,0,&value,nullptr);
    if(m_env!=nullptr)
        m_env->register_event(fd,EPOLLIN,this);
}

co_thread::~co_thread(){
    if(m_addr!=nullptr){
        free(m_addr);
    }
    if(m_local_state!=nullptr){
        free(m_local_state);
    }
}
