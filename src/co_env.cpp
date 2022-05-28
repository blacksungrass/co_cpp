#include "co_env.h"

/*
* x86-64 calling convention refer https://aaronbloomfield.github.io/pdr/book/x86-64bit-ccc-chapter.pdf
* caller need to save r10,r11 and parameters(rdi,rsi,rdx,rcx,r8,r9)
*/


co_thread::co_thread(co_thread_func_t func,void* args,co_env* env_p){
    m_env = env_p;
    const long stack_size = 1024;
    finished = false;
    
    m_addr = (long*)malloc(stack_size*sizeof(long));
    m_local_state = (long*)malloc(12*sizeof(void*));
    
    long ret_addr_idx = stack_size-8;
    if((long)(m_addr+ret_addr_idx)%16!=0){
        ret_addr_idx += 1;
    }
    
    m_addr[0] = (long)func;//rip
    m_addr[1] = (long)(m_addr+ret_addr_idx);//rsp
    //printf("rsp now is %x\n",m_addr[1]);
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
    cur_task_id = -1;
    epoll_fd = epoll_create(100);
}

int co_env::add_task(co_thread_func_t func,void* args){
    co_thread* p = new co_thread(func,args,this);
    ++cur_task_id;
    while(m_records.count(cur_task_id)){
        ++cur_task_id;
        if(cur_task_id<0){
            cur_task_id = 0;
        }
    }
    m_records[cur_task_id] = p;
    m_inv_records[p] = cur_task_id;
    add_list.insert(p);
    return cur_task_id;
}

void co_env::register_event(int fd,int event,co_thread* thread_ptr){
    epoll_event ev;
    ev.events = event;
    ev.data.ptr = new co_event_info(thread_ptr,fd);
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&ev);
}

void co_env::loop(){
    //Log("coenv start to loop");
    while(true){
        if(rd_list.empty()&&block_list.empty()&&add_list.empty()){
            break;
        }
        epoll_event events[200];
        int sleep_time = 0;
        if(rd_list.empty()&&add_list.empty()){
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
            if(cancel_list.count(*it)){//if in cancel list
                delete (*it);
                it = rd_list.erase(it);
                continue;
            }
            int val = (*it)->run();
            //printf("co_thread %x yield %d\n",*it,val);
            if(val==1){//if finished
                delete (*it);
                int task_id = m_inv_records[*it];
                m_inv_records.erase(*it);
                m_records.erase(task_id);
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
        remove_canceled_task();

        //append add_list
        rd_list.insert(add_list.begin(),add_list.end());
        add_list.clear();
    }
}

void co_env::remove_canceled_task(){
    auto it = block_list.begin();
    while(it!=block_list.end()){
        if(cancel_list.count(*it)){
            delete (*it);
            it = block_list.erase(it);
        }
        else{
            ++it;
        }
    }

    it = rd_list.begin();
    while(it!=rd_list.end()){
        if(cancel_list.count(*it)){
            delete (*it);
            it = rd_list.erase(it);
        }
        else{
            ++it;
        }
    }

    cancel_list.clear();
}

void co_env::cancel_task(int task_id){
    auto it = m_records.find(task_id);
    if(it!=m_records.end()){
        cancel_list.insert(it->second);
        m_inv_records.erase(it->second);
        m_records.erase(it);
    }
}