#pragma once

class co_env;

/*
* wrap a thread whose stack is on heap
* when it calls yeild, control flow will redirect into run function
* https://img-blog.csdn.net/20170510234951419?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvcXFfMTgyMTgzMzU=/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/Center
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