#include "co_env.h"
#include <algorithm>
#include <functional>
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
    //co_thread_events[thread_ptr].emplace_back(event_id);
    return event_id;
}

void co_env::handle_activated_cothread(co_thread* p){
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
        int val = (*it)->run();//run co_thread

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
        m_event_manager.handle_event([this](co_thread* p){
            this->handle_activated_cothread(p);
        },sleep_time);
        //printf("event num got from epoll:%d\n",num);
        
        process_rd_list();

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
