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
#include "co_event_manager.h"


//forward declares
class co_thread;
class co_event_manager;

using co_thread_func_t = void(*)(co_thread&,void*);

class co_env{
private:
    std::set<co_thread*> rd_list,block_list;
    std::set<co_thread*> cancel_list,add_list;
    co_event_manager m_event_manager;
    std::set<co_thread*> valid_cothreads;
    std::map<co_thread*,std::vector<long>> co_thread_events;

    void remove_canceled_task();
    void handle_activated_cothread(co_thread*);
    void process_rd_list();
    void remove_co_thread(co_thread*);
public:
    co_env();
    //add task and return a task id
    long add_task(co_thread_func_t,void*);
    long register_event(int fd,int event,co_thread* thread_ptr);
    void loop();
    void cancel_task(long);
    void cancel_event(long);
};


