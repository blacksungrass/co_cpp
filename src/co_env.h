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
    std::set<co_thread*> rd_list;//就绪任务集合
    std::set<co_thread*> block_list;//阻塞任务集合
    std::set<co_thread*> cancel_list;//取消任务集合
    std::set<co_thread*> add_list;//新增任务集合
    co_event_manager m_event_manager;//时间管理器
    std::set<co_thread*> valid_cothreads;
    std::map<co_thread*,int> co_thread_fd;
    std::map<co_thread*,int> co_thread_event;

    void remove_canceled_task();
    void handle_activated_cothread(co_thread*,int,int);
    void process_rd_list();
    void remove_co_thread(co_thread*);
public:
    //constructor
    co_env();

    //add task and return a task id
    long add_task(co_thread_func_t,void*);

    //register a event
    //when event triggered, the co_thread will move from block_list ot rd_list
    long register_event(int fd,int event,co_thread* thread_ptr);

    //do event loop
    void loop();

    //cancel task by task_id return by add_task
    void cancel_task(long);

    //cacel event by event_id return by register_event
    void cancel_event(long);

    void cancel_event(co_thread*);
};


