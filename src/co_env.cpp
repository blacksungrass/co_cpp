#include "co_env.h"

__attribute__((always_inline))
inline void co_thread::save_context(){
    __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(m_addr[0]));//save rip
    __asm__ __volatile__("mov %%rbp,%0":"=g"(m_addr[1]));//save rsp
    __asm__ __volatile__("mov (%%rbp),%0":"=g"(m_addr[2]));//save rbp
    __asm__ __volatile__("mov %%rdi,%0":"=g"(m_addr[3]));//save rdi
    __asm__ __volatile__("mov %%rsi,%0":"=g"(m_addr[4]));//save rsi
}

__attribute__((always_inline))
inline void co_thread::restore_context(){
    __asm__ __volatile__("mov %0,%%rsi"::"g"(m_addr[4]));//resotre rsi
    __asm__ __volatile__("mov %0,%%rdi"::"g"(m_addr[3]));//resotre rdi
    __asm__ __volatile__("mov %0,%%rsp"::"g"(m_addr[1]));//resotre rsp
    __asm__ __volatile__("mov %0,%%rbp;jmp *%1"::"g"(m_addr[2]),"r"(m_addr[0]));//put rbp and rip in the last
}

co_thread::co_thread(co_thread_func_t func,void* args,co_env* env_p){
    m_env = env_p;
    const long stack_size = 1024;
    finished = false;
    
    m_addr = (long*)malloc(stack_size*sizeof(long));
    
    long ret_addr_idx = stack_size-4;
    if((long)(m_addr+ret_addr_idx)%16!=0){
        ret_addr_idx += 1;
    }
    
    m_addr[0] = (long)func;//rip
    m_addr[1] = (long)(m_addr+ret_addr_idx);//rsp
    //printf("rsp now is %x\n",m_addr[1]);
    m_addr[2] = (long)(m_addr+ret_addr_idx);//rbp
    m_addr[3] = (long)(this);
    m_addr[4] = (long)(args);
    m_addr[ret_addr_idx+1] = (long)this;
    m_addr[ret_addr_idx] = (long)&finish;
}
//terminated or not
int co_thread::run(){
    if(this->finished){
        return 1;
    }
    __asm__ __volatile__("mov %%rbp,%0":"=g"(this->saved_rbp));
    restore_context();
    return 0;
}
void co_thread::yield(){
    save_context();
    __asm__ __volatile__("mov %0,%%rbp;mov $0,%%rax;leave;ret;"::"g"(this->saved_rbp));
    return;
}
void co_thread::yield_event(int fd,int event){
    save_context();

    if(m_env==nullptr){
        throw std::runtime_error("co_thread need a co_env to to yield_for some time");
    }
    m_env->register_event(fd,event,this);

    __asm__ __volatile__("mov %0,%%rbp;mov $2,%%rax;leave;ret;"::"g"(this->saved_rbp));
}
void co_thread::yield_for(int ms){
    save_context();

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

    __asm__ __volatile__("mov %0,%%rbp;mov $2,%%rax;leave;ret;"::"g"(this->saved_rbp));
}
void co_thread::suicide(){
    this->finished = true;
    __asm__ __volatile__("mov %0,%%rbp;mov $1,%%rax;leave;ret;"::"g"(this->saved_rbp));
}
void co_thread::finish(){
    co_thread* self;
    __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(self));
    self->finished = true;
    __asm__ __volatile__("mov %0,%%rbp;mov $1,%%rax;leave;ret;"::"g"(self->saved_rbp));
}
co_thread::~co_thread(){
    if(m_addr!=nullptr){
        free(m_addr);
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