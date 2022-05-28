#include "co_env.h"

/*
* this file is no longer used
* 本文件以不再使用
* 由于g++对于naked的实现,在naked函数里面只能使用全汇编，
* 所以我干脆把需要naked的函数直接用汇编在co_env.S中写了，
* 这样可以做到更加精细的控制，防止g++自动添加的一些汇编指
* 令导致逻辑错误。
* 不过由于本文件的可读性更强，逻辑更清晰，所以暂将本文件保留
*/

__attribute__((always_inline))
inline void co_thread::save_user_context(){
    asm(
        "mov 8(%rdi),%rax\n\t"//把m_local_state的指针保存在rax
        "mov (%rsp),%rcx\n\t"//把rip保存到rcx
        "mov %rcx,(%rax)\n\t"//m_local_state[0]=rip
        "mov %rsp,8(%rax)\n\t"//m_local_state[1]=rsp
        "mov %rbp,16(%rax)\n\t"//m_local_state[2]=rbp
        "mov %rdi,24(%rax)\n\t"//m_local_state[3]=rdi
        "mov %rsi,32(%rax)\n\t"//m_local_state[4]=rsi
        "mov %rbx,40(%rax)\n\t"//m_local_state[5]=rbx
        "mov %r12,48(%rax)\n\t"//m_local_state[6]=r12
        "mov %r13,56(%rax)\n\t"//m_local_state[7]=r13
        "mov %r14,64(%rax)\n\t"//m_local_state[8]=r14
        "mov %r15,72(%rax)\n\t"//m_local_state[9]=r15
    );
}

__attribute__((always_inline))
inline void co_thread::restore_user_context(){
    asm(
        "mov 8(%rdi),%rax\n\t"//把m_local_state的指针保存在rax
        "mov 72(%rax),%r15\n\t"//r15=m_local_state[9]
        "mov 64(%rax),%r14\n\t"//r14=m_local_state[8]
        "mov 56(%rax),%r13\n\t"//r13=m_local_state[7]
        "mov 48(%rax),%r12\n\t"//r12=m_local_state[6]
        "mov 40(%rax),%rbx\n\t"//rbx=m_local_state[5]
        "mov 32(%rax),%rsi\n\t"//rsi=m_local_state[4]
        "mov 24(%rax),%rdi\n\t"//rdi=m_local_state[3]
        "mov 16(%rax),%rbp\n\t"//rbp=m_local_state[2]
        "mov 8(%rax),%rsp\n\t"//rsp=m_local_state[1]
        //"mov (%rax),%rcx\n\t"//rcx=m_local_state[0](value=saved rip)
        "ret\n\t"//jmp m_local_state[0]
    );
}

__attribute__((always_inline))
inline void co_thread::save_cothread_context(){
    asm(
        "mov (%rdi),%rax\n\t"//把m_addr的指针保存在rax
        "mov (%rsp),%rcx\n\t"//把rip保存到rcx
        "mov %rcx,(%rax)\n\t"//m_addr[0]=rip
        "mov %rsp,8(%rax)\n\t"//m_addr[1]=rsp
        "mov %rbp,16(%rax)\n\t"//m_addr[2]=rbp
        "mov %rdi,24(%rax)\n\t"//m_addr[3]=rdi
        "mov %rsi,32(%rax)\n\t"//m_addr[4]=rsi
        "mov %rbx,40(%rax)\n\t"//m_addr[5]=rbx
        "mov %r12,48(%rax)\n\t"//m_local_state[6]=r12
        "mov %r13,56(%rax)\n\t"//m_local_state[7]=r13
        "mov %r14,64(%rax)\n\t"//m_local_state[8]=r14
        "mov %r15,72(%rax)\n\t"//m_local_state[9]=r15
    );
}

__attribute__((always_inline))
inline void co_thread::restore_cothread_context(){
    asm(
        "mov (%rdi),%rax\n\t"//把m_addr的指针保存在rax
        "mov 72(%rax),%r15\n\t"//r15=m_local_state[9]
        "mov 64(%rax),%r14\n\t"//r14=m_local_state[8]
        "mov 56(%rax),%r13\n\t"//r13=m_local_state[7]
        "mov 48(%rax),%r12\n\t"//r12=m_local_state[6]
        "mov 40(%rax),%rbx\n\t"//rbx=m_addr[5]
        "mov 32(%rax),%rsi\n\t"//rsi=m_addr[4]
        "mov 24(%rax),%rdi\n\t"//rdi=m_addr[3]
        "mov 16(%rax),%rbp\n\t"//rbp=m_addr[2]
        "mov 8(%rax),%rsp\n\t"//rsp=m_addr[1]
        "ret\n\t"//jmp m_addr[0]
    );
}


co_thread::co_thread(co_thread_func_t func,void* args,co_env* env_p){
    m_env = env_p;
    const long stack_size = 1024;
    finished = false;
    
    m_addr = (long*)malloc(stack_size*sizeof(long));
    printf("m_addr=%p\n",m_addr);
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


//terminated or not
__attribute__ ((naked))
int co_thread::run(){
    save_user_context();
    //call run_impl
    __asm__ __volatile__("mov %0,%%rdi;call *%1"::"g"(this),"g"(run_impl));
    asm("test eax,eax\n\t"
        "jz return\n\t"
        "jmp continue\n\t"
        "return:ret\n\t"
        "continue:nop\n\t");
    restore_cothread_context();
}

__attribute__ ((naked))
void co_thread::yield(){
    save_cothread_context();
    asm("mov $0,%rax\n\t");
    restore_user_context();
}


void co_thread::yield_events(int fds[],int events[],int n){
    save_cothread_context();

    yield_events_impl(fds,events,n);

    __asm__ __volatile__("mov %0,%%rdi"::"g"(this));
    restore_user_context();
    asm("mov $2,%rax\n\t"
        "ret\n\t");
}

__attribute__ ((naked))
void co_thread::yield_event(int fd,int event){
    save_cothread_context();

    yield_event_impl(fd,event);//todo 为什么在release模式下这个函数没有保持rdi不变，违反了nonvolatile啊？？难道naked函数的内层函数也默认naked吗？

    __asm__ __volatile__("mov %0,%%rdi"::"g"(this));
    restore_user_context();
    asm("mov $2,%rax\n\t"
        "ret\n\t");
}


void co_thread::yield_for(int ms){
    save_cothread_context();

    yield_for_impl(ms);

    __asm__ __volatile__("mov %0,%%rdi"::"g"(this));
    restore_user_context();
    asm("mov $2,%rax\n\t"
        "ret\n\t");
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

void co_thread::suicide(){
    this->finished = true;
    restore_user_context();
    asm("mov $1,%rax\n\t"
        "ret\n\t");
}

void co_thread::finish(){
    co_thread* self;
    __asm__ __volatile__("mov (%%rsp),%0":"=g"(self));
    self->finished = true;
    self->restore_user_context();
    asm("mov $1,%rax\n\t"
        "ret\n\t");
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