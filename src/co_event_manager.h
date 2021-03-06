#pragma once
#include "co_thread.h"
#include <functional>
#include <sys/epoll.h>
#include <set>
#include <vector>
#include <map>

class co_thread;
class event_info_node;
class co_event_manager{
private:
    int epoll_fd;
    event_info_node* head;//main data
    std::map<int,std::set<event_info_node*>> fd_record;//fd as index,fd:event_info_node(event,co_thread,fd)
    std::map<co_thread*,std::set<event_info_node*>> co_thread_record;//co_thread* as index
    std::map<int,int> event_cache;//fd:events cache keep sync with epoll internal status
    std::set<event_info_node*> valid_event;

    void delete_event(long,bool,bool);
    void add_node(event_info_node*);
    void del_node(event_info_node*);
public:
    //constructor
    co_event_manager();

    //add a event and return event_id
    long add_event(int fd, int event, co_thread* ptr);

    //cancel a event by event_id returned by add_event
    void cancel_event(long);

    //cancel a co_thread's all event
    void cancel_co_thread(co_thread*);

    //handel events
    void handle_event(std::function<void(co_thread*,int,int)> func,int timeout);
};