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
    long saved_rbp;
    __attribute__((always_inline))
    inline void save_context(){
        __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(m_addr[0]));//save rip
        __asm__ __volatile__("mov %%rbp,%0":"=g"(m_addr[1]));//save rsp
        __asm__ __volatile__("mov (%%rbp),%0":"=g"(m_addr[2]));//save rbp
    }
    __attribute__((always_inline))
    inline void restore_context(){
        __asm__ __volatile__("mov %0,%%rdi"::"g"(this));

        __asm__ __volatile__("mov %0,%%rsp"::"g"(m_addr[1]));
        __asm__ __volatile__("mov %0,%%rbp;jmp *%1"::"g"(m_addr[2]),"r"(m_addr[0]));
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
    }
    //terminated or not
    bool run(){
        if(this->finished){
            return true;
        }
        __asm__ __volatile__("mov %%rbp,%0":"=g"(this->saved_rbp));
        restore_context();
        return false;
    }
    void yield(){
        save_context();
        __asm__ __volatile__("mov %0,%%rbp;mov %%rbp,%%rsp"::"g"(this->saved_rbp));
        __asm__ __volatile__("mov $0,%rax");
        return;
    }
    static void finish(){
        co_thread* self;
        __asm__ __volatile__("mov 8(%%rbp),%0":"=g"(self));
        self->finished = true;
         __asm__ __volatile__("mov %0,%%rbp;mov %%rbp,%%rsp"::"g"(self->saved_rbp));
        __asm__ __volatile__("mov $1,%rax");
        return;
    }
};
