#pragma once

class co_env;

/*
* wrap a thread whose stack is on heap
* when it calls yeild, control flow will redirect into run function
*/
class co_thread{
private:
    using co_thread_func_t = void(*)(co_thread&,void*);
    long* m_addr;//协程栈空间
    long* m_local_state;//保存切换协程时的本地线程，至少9*8=72个字节大小
    bool finished;
    long saved_rbp;
    co_env* m_env;

    void yield_event_impl(int fd,int event);
    void yield_events_impl(int fds[],int events[],int n);
    void yield_for_impl(int ms);
    void yield_impl();
    int run_impl();
public:
    co_thread(co_thread_func_t func,void* args,co_env* env_p=nullptr);
    
    //parameter event_and_fd: the event and fd which awaked the cothread
    //event at hight 32 digits, and fd at low 32 digits
    //if start the first time, event default to 0
    //return 0:normal yield
    //return 1:finish(exited)
    //return 2:blocked
    int run(long event_and_fd=0);

    long yield(bool blocked=false);

    int yield_event(int fd,int event);

    int yield_events(int fds[],int events[],int n);
    
    int yield_for(int ms);

    void suicide();

    static void finish();

    void cancel_all_event();

    void cancel_event(long);

    long register_event(int fd,int event);

    void register_events(int fds[],int events[],int n);

    ~co_thread();
};