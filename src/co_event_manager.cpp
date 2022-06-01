#include "co_event_manager.h"
#include "co_env.h"
#include <algorithm>

class event_info_node{
public:
    int fd;
    int events;
    co_thread* thread_ptr;
    event_info_node* next;
    event_info_node* prev;
    event_info_node(int f,int e,co_thread* p){
        fd = f;
        events = e;
        thread_ptr = p;
        next = nullptr;
        prev = nullptr;
    }
};

co_event_manager::co_event_manager(){
    epoll_fd = epoll_create(100);
    head = nullptr;
}

//在双链表中添加节点
void co_event_manager::add_node(event_info_node* node){
    if(head!=nullptr){
        node->next = head;
        node->prev = head->prev;
        head->prev->next = node;
        head->prev = node;
        head = node;
    }
    else{
        node->next = node;
        node->prev = node;
        head = node;
    }
}

//在双链表中删除并释放节点
void co_event_manager::del_node(event_info_node* node){
    if(node->prev!=node){//if not the only node of double link list
        if(node==head){
            head = node->next;
        }
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    else{
        head = nullptr;
    }
    delete node;
}

long co_event_manager::add_event(int fd,int events,co_thread* ptr){
    event_info_node* node = new event_info_node(fd,events,ptr);
    add_node(node);

    co_thread_record[ptr].insert(node);
    fd_record[fd].insert(node);
    valid_event.insert(head);

    auto it = event_cache.find(fd);
    if(it!=end(event_cache)){
        int cache = it->second;
        int intersecion = cache&events;
        if(events&(~intersecion)){
            epoll_event ev;
            ev.data.fd = fd;
            ev.events = cache|events;
            epoll_ctl(epoll_fd,EPOLL_CTL_MOD,fd,&ev);
        }
    }
    else{
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&ev);
    }
    
    return (long)(head);
}

void co_event_manager::delete_event(long event_id,bool modify_fd_record,bool modify_co_thread_record){
    event_info_node* node = reinterpret_cast<event_info_node*>(event_id);
    if(valid_event.count(node)==0)
        return;
    valid_event.erase(node);
    int fd = node->fd;
    co_thread* thread_ptr = node->thread_ptr;
    if(modify_fd_record){
        fd_record[fd].erase(node);
        if(fd_record[fd].empty()){
            fd_record.erase(fd);
        }
    }

    if(modify_co_thread_record){
        co_thread_record[thread_ptr].erase(node);
        if(co_thread_record[thread_ptr].empty()){
            co_thread_record.erase(thread_ptr);
        }
    }
    int events = 0;

    for(event_info_node* p:fd_record[fd]){
        if(p!=node)
            events |= p->events;
    }
   
    auto cache_it = event_cache.find(fd);

    if(cache_it==end(event_cache)||cache_it->second!=events){
        if(events==0){
            event_cache.erase(fd);
            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,nullptr);
        }
        else{
            event_cache[fd] = events;
            epoll_event ev;
            ev.data.fd = fd;
            ev.events = events;
            epoll_ctl(epoll_fd,EPOLL_CTL_MOD,fd,&ev);
        }
    }

    del_node(node);
}

void co_event_manager::cancel_event(long event_id){
    delete_event(event_id,true,true);
}

static void erase_if(std::set<event_info_node*>& set, std::function<bool(event_info_node*)> pred){
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

void co_event_manager::handle_event(std::function<void(co_thread*,int,int)> func,int timeout){
    epoll_event events[200];
    int ret = epoll_wait(epoll_fd,events,200,timeout);
    for(int i=0;i<ret;++i){
        int fd = events[i].data.fd;
        erase_if(fd_record[fd],[&](event_info_node* node)->bool{
            int ev = EPOLLERR|EPOLLHUP;//this two event need treat specially
            ev = ev&(events[i].events);
            ev = ev|(node->events&events[i].events);
            if(ev){
                func(node->thread_ptr,fd,ev);
                delete_event((long)node,false,true);
                return true;
            }
            else{
                return false;
            }
        });
        if(fd_record[fd].empty()){
            fd_record.erase(fd);
        }
    }
}

void co_event_manager::cancel_co_thread(co_thread* p){
    for(auto node:co_thread_record[p]){
        delete_event((long)node,true,false);
    }
    co_thread_record.erase(p);
}