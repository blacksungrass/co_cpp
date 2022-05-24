#pragma once
#include <vector>
#include <cstdlib>

/*
* wrap a thread whose stack is on heap
* when it calls yeild, control flow will redirect into run function
*/
class co_thread{
private:
    long* m_addr;
    bool finished;
    long saved_rsp;
    inline void save_context(){
        printf("save_context\n");
    }
    inline void restore_context(){
        printf("restore_context\n");
    }
public:
    co_thread(void* func){
        finished = false;
        m_addr = (long*)malloc(1024);
        m_addr[0] = (long)func;//rip
        m_addr[1] = (long)(m_addr+1022);//rsp
        m_addr[2] = (long)(m_addr+1022);//rbp
        m_addr[1023] = (long)this;
        m_addr[1022] = (long)&finish;
        printf("1022:(%x)=%x\n",m_addr+1022,&finish);
        printf("1023:(%x)=%x\n",m_addr+1023,this);
    }
    //terminated or not
    bool run(){
        if(this->finished){
            return true;
        }
        __asm__ __volatile__("mov %%rsp,%0":"=g"(this->saved_rsp));
        long ip_value = m_addr[0];
        //printf("get ip_value in function run:%x\n",ip_value);
        long rsp_value = m_addr[1];
        long rbp_value = m_addr[2];
        __asm__ __volatile__("mov %0,%%rsp;mov %2,%%rdi;mov %3,%%rbp;jmp *%1"::"g"(rsp_value),"r"(ip_value),"g"(this),"g"(rbp_value));
        return false;
    }
    void yield(){
        long ip_value;
        long rsp_value;
        long rbp_value;
        __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(ip_value));
        __asm__ __volatile__("mov %%rbp,%0":"=g"(rsp_value));
        __asm__ __volatile__("mov (%%rbp),%0":"=g"(rbp_value));
        m_addr[0] = ip_value;
        //printf("set ip_value in function yield:%x\n",ip_value);
        m_addr[1] = rsp_value;
        m_addr[2] = rbp_value;
        __asm__ __volatile__("mov %0,%%rsp"::"g"(this->saved_rsp));
        __asm__ __volatile__("mov $0,%rax");
        return;
    }
    static void finish(){
        co_thread* self;
        __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(self));
        self->finished = true;
        __asm__ __volatile__("mov %0,%%rsp"::"g"(self->saved_rsp));
        __asm__ __volatile__("mov $1,%rax");
        return;
    }
};
