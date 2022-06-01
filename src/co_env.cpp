#include "co_env.h"
#include <algorithm>
#include <functional>
#include "co_thread.h"


static long make_event_and_fd(int event,int fd){
    long ret;
    long levent = event;
    long lfd = fd;
    ret = (levent<<32)+lfd;
    return ret;
}

co_env::co_env(){
}

long co_env::add_task(co_thread_func_t func,void* args){
    co_thread* p = new co_thread(func,args,this);
    add_list.insert(p);
    valid_cothreads.insert(p);
    return (long)p;
}

void co_env::remove_co_thread(co_thread* ptr){
    if(valid_cothreads.count(ptr)==0)
        return;
    valid_cothreads.erase(ptr);
    m_event_manager.cancel_co_thread(ptr);
    delete ptr;
}

long co_env::register_event(int fd,int event,co_thread* thread_ptr){
    long event_id = m_event_manager.add_event(fd,event,thread_ptr);
    return event_id;
}

void co_env::handle_activated_cothread(co_thread* p,int fd, int event){
    //how to handle EPOLLERR and EPOLLHUB?
    if(block_list.count(p)){
        block_list.erase(p);
        rd_list.insert(p);
    }
}

void co_env::process_rd_list(){
    auto it = rd_list.begin();
    while(it!=rd_list.end()){
        if(cancel_list.count(*it)){//if in cancel list
            remove_co_thread(*it);
            it = rd_list.erase(it);
            continue;
        }
        int fd = co_thread_fd[*it];
        int event = co_thread_event[*it];
        long event_and_fd = make_event_and_fd(event,fd);
        //printf("event_and_fd:%lld event:%d fd:%d\n",event_and_fd,event,fd);
        int val = (*it)->run(event_and_fd);//run co_thread

        if(val==1){//if finished
            remove_co_thread(*it);
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

void co_env::loop(){
    //Log("coenv start to loop");
    while(true){
        if(rd_list.empty()&&block_list.empty()&&add_list.empty()){
            break;
        }
        int sleep_time = 0;
        if(rd_list.empty()&&add_list.empty()){
            sleep_time = -1;
        }
        //printf("rd_list_len=%d\tblock_list_len=%d\n",rd_list.size(),block_list.size());
        m_event_manager.handle_event([this](co_thread* p,int fd, int event){
            this->handle_activated_cothread(p,fd,event);
            co_thread_fd.emplace(p,fd);
            co_thread_event.emplace(p,event);
        },sleep_time);
        
        process_rd_list();
        co_thread_fd.clear();
        co_thread_event.clear();

        //append add_list
        rd_list.insert(add_list.begin(),add_list.end());
        add_list.clear();

        remove_canceled_task();
    }
}


static void erase_if(std::set<co_thread*>& set, std::function<bool(co_thread*)> pred){
    auto it = begin(set);
    while(it!=end(set)){
        if(pred(*it)){
            it = set.erase(it);
        }
        else{
            ++it;
        }
    }
}

/*
移除已经取消了的任务(co_thread)
*/
void co_env::remove_canceled_task(){
    erase_if(block_list,[&](co_thread* p)->bool{
        if(cancel_list.count(p)){
            remove_co_thread(p);
            return true;
        }
        else{
            return false;
        }
    });
    erase_if(rd_list,[&](co_thread* p){
        if(cancel_list.count(p)){
            remove_co_thread(p);
            return true;
        }
        else{
            return false;
        }
    });
    cancel_list.clear();
}

void co_env::cancel_task(long task_id){
    co_thread* p = (co_thread*)(task_id);
    if(valid_cothreads.count(p)){
        cancel_list.insert(p);
    }
}

void co_env::cancel_event(long event_id){
    m_event_manager.cancel_event(event_id);
}

void co_env::cancel_event(co_thread* thread){
    if(valid_cothreads.count(thread)==0)
        return;
    m_event_manager.cancel_co_thread(thread);
}