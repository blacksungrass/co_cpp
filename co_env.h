#pragma once
#include <vector>
#include <exception>
#include <set>
#include <memory>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/timerfd.h>
//#include <sys/time.h>
//#include <unistd.h>
//#include <time.h>

//forward declares
class co_thread;

using co_thread_func_t = void(*)(co_thread&,void*);

class co_env{
private:
    std::set<co_thread*> rd_list,block_list;
    int epoll_fd;
    int cur;
public:
    co_env();
    void add_task(co_thread_func_t,void*);
    void register_event(int fd,int event,co_thread* thread_ptr);
    void loop();
};


/*
* wrap a thread whose stack is on heap
* when it calls yeild, control flow will redirect into run function
* https://img-blog.csdn.net/20170510234951419?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvcXFfMTgyMTgzMzU=/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/Center
*/
class co_thread{
private:
    long* m_addr;
    bool finished;
    long saved_rbp;
    co_env* m_env;

    __attribute__((always_inline))
    inline void save_context(){
        __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(m_addr[0]));//save rip
        __asm__ __volatile__("mov %%rbp,%0":"=g"(m_addr[1]));//save rsp
        __asm__ __volatile__("mov (%%rbp),%0":"=g"(m_addr[2]));//save rbp
        __asm__ __volatile__("mov %%rdi,%0":"=g"(m_addr[3]));//save rdi
        __asm__ __volatile__("mov %%rsi,%0":"=g"(m_addr[4]));//save rsi
    }

    __attribute__((always_inline))
    inline void restore_context(){
        __asm__ __volatile__("mov %0,%%rsi"::"g"(m_addr[4]));//resotre rsi
        __asm__ __volatile__("mov %0,%%rdi"::"g"(m_addr[3]));//resotre rdi
        __asm__ __volatile__("mov %0,%%rsp"::"g"(m_addr[1]));//resotre rsp
        __asm__ __volatile__("mov %0,%%rbp;jmp *%1"::"g"(m_addr[2]),"r"(m_addr[0]));//put rbp and rip in the last
    }
public:
    
    co_thread(co_thread_func_t func,void* args,co_env* env_p=nullptr){
        m_env = env_p;
        const long stack_size = 1024;
        finished = false;
       
        m_addr = (long*)malloc(stack_size*sizeof(long));
        
        long ret_addr_idx = stack_size-4;
        if((long)(m_addr+ret_addr_idx)%16!=0){
            ret_addr_idx += 1;
        }
        
        m_addr[0] = (long)func;//rip
        m_addr[1] = (long)(m_addr+ret_addr_idx);//rsp
        //printf("rsp now is %x\n",m_addr[1]);
        m_addr[2] = (long)(m_addr+ret_addr_idx);//rbp
        m_addr[3] = (long)(this);
        m_addr[4] = (long)(args);
        m_addr[ret_addr_idx+1] = (long)this;
        m_addr[ret_addr_idx] = (long)&finish;
    }
    //terminated or not
    int run(){
        if(this->finished){
            return 1;
        }
        __asm__ __volatile__("mov %%rbp,%0":"=g"(this->saved_rbp));
        restore_context();
        return 0;
    }
    void yield(){
        save_context();
        __asm__ __volatile__("mov %0,%%rbp;mov $0,%%rax;leavel;ret;"::"g"(this->saved_rbp));
        return;
    }
    void yield_event(int fd,int event){
        save_context();

        if(m_env==nullptr){
            throw std::runtime_error("co_thread need a co_env to to yield_for some time");
        }
        m_env->register_event(fd,event,this);

        __asm__ __volatile__("mov %0,%%rbp;mov $2,%%rax;leave;ret;"::"g"(this->saved_rbp));
    }
    void yield_for(int ms){
        save_context();

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

        __asm__ __volatile__("mov %0,%%rbp;mov $2,%%rax;leave;ret;"::"g"(this->saved_rbp));
    }
    void suicide(){
        this->finished = true;
        __asm__ __volatile__("mov %0,%%rbp;mov $1,%%rax;leave;ret;"::"g"(this->saved_rbp));
    }
    static void finish(){
        co_thread* self;
        __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(self));
        self->finished = true;
        __asm__ __volatile__("mov %0,%%rbp;mov $1,%%rax;leave;ret;"::"g"(self->saved_rbp));
    }
    ~co_thread(){
        if(m_addr!=nullptr){
            free(m_addr);
        }
    }
};

class co_event_info{
public:
    co_thread* thread_ptr;
    int fd;
    co_event_info(co_thread* ptr,int event_fd){
        thread_ptr = ptr;
        fd = event_fd;
    }
};


co_env::co_env(){
    cur=-1;
    epoll_fd = epoll_create(100);
}

void co_env::add_task(co_thread_func_t func,void* args){
    rd_list.insert(new co_thread(func,args,this));
}

void co_env::register_event(int fd,int event,co_thread* thread_ptr){
    epoll_event ev;
    ev.events = event;
    ev.data.ptr = new co_event_info(thread_ptr,fd);
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&ev);
}

void co_env::loop(){
    while(true){
        if(rd_list.empty()&&block_list.empty()){
            break;
        }
        epoll_event events[200];
        int sleep_time = 0;
        if(rd_list.empty()){
            sleep_time = -1;
        }
        //printf("rd_list_len=%d\tblock_list_len=%d\n",rd_list.size(),block_list.size());
        int num = epoll_wait(epoll_fd,events,200,sleep_time);
        //printf("event num got from epoll:%d\n",num);
        for(int i=0;i<num;++i){
            co_event_info* info_ptr = (co_event_info*)(events[i].data.ptr);
            co_thread* thread_ptr = info_ptr->thread_ptr;
            int fd = info_ptr->fd;
            if(block_list.count(thread_ptr)){
                block_list.erase(thread_ptr);
                rd_list.insert(thread_ptr);
            }
            delete info_ptr;
            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,nullptr);
        }
        auto it = rd_list.begin();
        while(it!=rd_list.end()){
            int val = (*it)->run();
            //printf("co_thread %x yield %d\n",*it,val);
            if(val==1){//if finished
                delete (*it);
                it = rd_list.erase(it);
            }
            else if(val==2){//if blocked
                block_list.insert(*it);
                it = rd_list.erase(it);
            }
            else{//normal
                ++it;
            }
        }
    }
}
