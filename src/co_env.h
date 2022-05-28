#pragma once
#include <vector>
#include <exception>
#include <set>
#include <memory>
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <map>
#include "log.h"


//forward declares
class co_thread;

using co_thread_func_t = void(*)(co_thread&,void*);

class co_env{
private:
    std::set<co_thread*> rd_list,block_list;
    int epoll_fd;
    int cur_task_id;
    std::map<int,co_thread*> m_records;
    std::map<co_thread*,int> m_inv_records;
    std::set<co_thread*> cancel_list,add_list;
    void remove_canceled_task();
public:
    co_env();
    //add task and return a task id
    int add_task(co_thread_func_t,void*);
    void register_event(int fd,int event,co_thread* thread_ptr);
    void loop();
    void cancel_task(int);
};


/*
* wrap a thread whose stack is on heap
* when it calls yeild, control flow will redirect into run function
* https://img-blog.csdn.net/20170510234951419?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvcXFfMTgyMTgzMzU=/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/Center
*/
class co_thread{
private:
    long* m_addr;//协程栈空间
    long* m_local_state;//保存切换协程时的本地线程，至少9*8=72个字节大小
    bool finished;
    long saved_rbp;
    co_env* m_env;

    /*
    __attribute__((always_inline))
    inline void restore_user_context();

    __attribute__((always_inline))
    inline void restore_cothread_context();

    __attribute__((always_inline))
    inline void save_user_context();

    __attribute__((always_inline))
    inline void save_cothread_context();
    */

    void yield_event_impl(int fd,int event);
    void yield_events_impl(int fds[],int events[],int n);
    void yield_for_impl(int ms);
    void yield_impl();
    int run_impl();
public:
    co_thread(co_thread_func_t func,void* args,co_env* env_p=nullptr);
    
    //terminated or not
    int run();

    void yield();

    void yield_event(int fd,int event);

    void yield_events(int fds[],int events[],int n);
    
    void yield_for(int ms);

    void suicide();

    static void finish();

    ~co_thread();
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
