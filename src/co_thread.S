.global _ZN9co_thread3runEl                 # co_thread::run(long) done
.global _ZN9co_thread6finishEv              # co_thread::finish() done
.global _ZN9co_thread11yield_eventEii       # co_thread::yield_event(int, int) done
.global _ZN9co_thread12yield_eventsEPiS0_i  # co_thread::yield_events(int*, int*, int)
.global _ZN9co_thread5yieldEb               # co_thread::yield(bool) done
.global _ZN9co_thread7suicideEv             # co_thread::suicide() done
.global _ZN9co_thread9yield_forEi           # co_thread::yield_for(int) done


# _ZN9co_thread10yield_implEv               co_thread::yield_impl()
# _ZN9co_thread14yield_for_implEi           co_thread::yield_for_impl(int)
# _ZN9co_thread16yield_event_implEii        co_thread::yield_event_impl(int, int)
# _ZN9co_thread17yield_events_implEPiS0_i   co_thread::yield_events_impl(int*, int*, int)
# _ZN9co_thread8run_implEv                  co_thread::run_impl()


#co_thread::run(long)
_ZN9co_thread3runEl:
        #save_user_context
        mov 8(%rdi),%rax  # 把m_local_state的指针保存在rax
        mov (%rsp),%rcx   # 把rip保存到rcx
        mov %rcx,(%rax)   # m_local_state[0]=rip
        mov %rsp,8(%rax)  # m_local_state[1]=rsp
        mov %rbp,16(%rax) # m_local_state[2]=rbp
        mov %rdi,24(%rax) # m_local_state[3]=rdi
        mov %rsi,32(%rax) # m_local_state[4]=rsi
        mov %rbx,40(%rax) # m_local_state[5]=rbx
        mov %r12,48(%rax) # m_local_state[6]=r12
        mov %r13,56(%rax) # m_local_state[7]=r13
        mov %r14,64(%rax) # m_local_state[8]=r14
        mov %r15,72(%rax) # m_local_state[9]=r15
        
        #call co_thread::run_impl()
        #rdi haven't change ,so no need to set rdi
        push %rdi
        call _ZN9co_thread8run_implEv
        pop %rdi

        test %eax,%eax
        jnz return
        jmp switch
return:
        ret
switch: 
        #restore_cothread_context
        mov (%rdi),%rax   # 把m_addr的指针保存在rax
        mov 72(%rax),%r15 # r15=m_addr[9]
        mov 64(%rax),%r14 # r14=m_addr[8]
        mov 56(%rax),%r13 # r13=m_addr[7]
        mov 48(%rax),%r12 # r12=m_addr[6]
        mov 40(%rax),%rbx # rbx=m_addr[5]
        mov 32(%rax),%rsi # rsi=m_addr[4]
        mov 24(%rax),%rdi # rdi=m_addr[3]
        mov 16(%rax),%rbp # rbp=m_addr[2]
        mov 8(%rax),%rsp  # rsp=m_addr[1]

        #set rax=rsi(event parameter saved before)
        mov 8(%rdi),%rax
        mov 32(%rax),%rax

        ret               # ret to m_addr[0]

# co_thread::yield(bool blocked=false)
_ZN9co_thread5yieldEb:
        #save_cothread_context
        mov (%rdi),%rax   # 把m_addr的指针保存在rax
        mov (%rsp),%rcx   # 把rip保存到rcx
        mov %rcx,(%rax)   # m_addr[0]=rip
        mov %rsp,8(%rax)  # m_addr[1]=rsp
        mov %rbp,16(%rax) # m_addr[2]=rbp
        mov %rdi,24(%rax) # m_addr[3]=rdi
        mov %rsi,32(%rax) # m_addr[4]=rsi
        mov %rbx,40(%rax) # m_addr[5]=rbx
        mov %r12,48(%rax) # m_addr[6]=r12
        mov %r13,56(%rax) # m_addr[7]=r13
        mov %r14,64(%rax) # m_addr[8]=r14
        mov %r15,72(%rax) # m_addr[9]=r15

        #call co_thread::yield_impl()
        push %rdi
        call _ZN9co_thread10yield_implEv
        pop %rdi
        
        #restore_user_context
        mov 8(%rdi),%rax # 把m_local_state的指针保存在rax
        mov 72(%rax),%r15 # r15=m_local_state[9]
        mov 64(%rax),%r14 # r14=m_local_state[8]
        mov 56(%rax),%r13 # r13=m_local_state[7]
        mov 48(%rax),%r12 # r12=m_local_state[6]
        mov 40(%rax),%rbx # rbx=m_local_state[5]
        mov 32(%rax),%rsi # rsi=m_local_state[4]
        mov 24(%rax),%rdi # rdi=m_local_state[3]
        mov 16(%rax),%rbp # rbp=m_local_state[2]
        mov 8(%rax),%rsp  # rsp=m_local_state[1]

        #set return value
        mov (%rdi),%rax
        mov 32(%rax),%rax #set blocked parameter (rsi) to rax
        test %al,%al
        jnz blocked
        mov $0,%eax      #set return value 0
        ret
blocked:mov $2,%eax      #set return value 2
        ret

# co_thread::finish()
_ZN9co_thread6finishEv:
        mov (%rsp),%rdi     #get "this" pointer
        movb $1,16(%rdi)    #this->finised=true

        #restore_user_context
        mov 8(%rdi),%rax  # 把m_local_state的指针保存在rax
        mov 72(%rax),%r15 # r15=m_local_state[9]
        mov 64(%rax),%r14 # r14=m_local_state[8]
        mov 56(%rax),%r13 # r13=m_local_state[7]
        mov 48(%rax),%r12 # r12=m_local_state[6]
        mov 40(%rax),%rbx # rbx=m_local_state[5]
        mov 32(%rax),%rsi # rsi=m_local_state[4]
        mov 24(%rax),%rdi # rdi=m_local_state[3]
        mov 16(%rax),%rbp # rbp=m_local_state[2]
        mov 8(%rax),%rsp  # rsp=m_local_state[1]

        #set return value
        mov $1,%eax 
        ret

# co_thread::yield_event(int, int)
_ZN9co_thread11yield_eventEii:
        #save_cothread_context
        mov (%rdi),%rax   # 把m_addr的指针保存在rax
        mov (%rsp),%rcx   # 把rip保存到rcx
        mov %rcx,(%rax)   # m_addr[0]=rip
        mov %rsp,8(%rax)  # m_addr[1]=rsp
        mov %rbp,16(%rax) # m_addr[2]=rbp
        mov %rdi,24(%rax) # m_addr[3]=rdi
        mov %rsi,32(%rax) # m_addr[4]=rsi
        mov %rbx,40(%rax) # m_addr[5]=rbx
        mov %r12,48(%rax) # m_addr[6]=r12
        mov %r13,56(%rax) # m_addr[7]=r13
        mov %r14,64(%rax) # m_addr[8]=r14
        mov %r15,72(%rax) # m_addr[9]=r15

        #call co_thread::yield_event_impl(int, int)
        #rdi,rsi,rdx didn't change, no need to reset parameters
        push %rdi
        call _ZN9co_thread16yield_event_implEii
        pop %rdi

        #restore_user_context
        mov 8(%rdi),%rax  # 把m_local_state的指针保存在rax
        mov 72(%rax),%r15 # r15=m_local_state[9]
        mov 64(%rax),%r14 # r14=m_local_state[8]
        mov 56(%rax),%r13 # r13=m_local_state[7]
        mov 48(%rax),%r12 # r12=m_local_state[6]
        mov 40(%rax),%rbx # rbx=m_local_state[5]
        mov 32(%rax),%rsi # rsi=m_local_state[4]
        mov 24(%rax),%rdi # rdi=m_local_state[3]
        mov 16(%rax),%rbp # rbp=m_local_state[2]
        mov 8(%rax),%rsp  # rsp=m_local_state[1]

        #set return value
        mov $2,%eax
        ret

# co_thread::yield_for(int)
_ZN9co_thread9yield_forEi:
        #save_cothread_context
        mov (%rdi),%rax   # 把m_addr的指针保存在rax
        mov (%rsp),%rcx   # 把rip保存到rcx
        mov %rcx,(%rax)   # m_addr[0]=rip
        mov %rsp,8(%rax)  # m_addr[1]=rsp
        mov %rbp,16(%rax) # m_addr[2]=rbp
        mov %rdi,24(%rax) # m_addr[3]=rdi
        mov %rsi,32(%rax) # m_addr[4]=rsi
        mov %rbx,40(%rax) # m_addr[5]=rbx
        mov %r12,48(%rax) # m_addr[6]=r12
        mov %r13,56(%rax) # m_addr[7]=r13
        mov %r14,64(%rax) # m_addr[8]=r14
        mov %r15,72(%rax) # m_addr[9]=r15

        #call co_thread::yield_for_impl(int)
        #rdi,rsi didn't change
        push %rdi
        call _ZN9co_thread14yield_for_implEi     
        pop %rdi 

        #restore_user_context
        mov 8(%rdi),%rax  # 把m_local_state的指针保存在rax
        mov 72(%rax),%r15 # r15=m_local_state[9]
        mov 64(%rax),%r14 # r14=m_local_state[8]
        mov 56(%rax),%r13 # r13=m_local_state[7]
        mov 48(%rax),%r12 # r12=m_local_state[6]
        mov 40(%rax),%rbx # rbx=m_local_state[5]
        mov 32(%rax),%rsi # rsi=m_local_state[4]
        mov 24(%rax),%rdi # rdi=m_local_state[3]
        mov 16(%rax),%rbp # rbp=m_local_state[2]
        mov 8(%rax),%rsp  # rsp=m_local_state[1]

        #set return value 2
        mov $2,%eax
        ret

# co_thread::suicide()
_ZN9co_thread7suicideEv:
        #set this->finished=true
        movb $1,16(%rdi)

        #restore_user_context
        mov 8(%rdi),%rax  # 把m_local_state的指针保存在rax
        mov 72(%rax),%r15 # r15=m_local_state[9]
        mov 64(%rax),%r14 # r14=m_local_state[8]
        mov 56(%rax),%r13 # r13=m_local_state[7]
        mov 48(%rax),%r12 # r12=m_local_state[6]
        mov 40(%rax),%rbx # rbx=m_local_state[5]
        mov 32(%rax),%rsi # rsi=m_local_state[4]
        mov 24(%rax),%rdi # rdi=m_local_state[3]
        mov 16(%rax),%rbp # rbp=m_local_state[2]
        mov 8(%rax),%rsp  # rsp=m_local_state[1]

        #set return value 1
        mov $1,%eax
        ret

# co_thread::yield_events(int*, int*, int)
_ZN9co_thread12yield_eventsEPiS0_i:
        #save rcx
        push %rcx

        #save_cothread_context
        mov (%rdi),%rax   # 把m_addr的指针保存在rax
        mov (%rsp),%rcx   # 把rip保存到rcx
        mov %rcx,(%rax)   # m_addr[0]=rip
        mov %rsp,8(%rax)  # m_addr[1]=rsp
        mov %rbp,16(%rax) # m_addr[2]=rbp
        mov %rdi,24(%rax) # m_addr[3]=rdi
        mov %rsi,32(%rax) # m_addr[4]=rsi
        mov %rbx,40(%rax) # m_addr[5]=rbx
        mov %r12,48(%rax) # m_addr[6]=r12
        mov %r13,56(%rax) # m_addr[7]=r13
        mov %r14,64(%rax) # m_addr[8]=r14
        mov %r15,72(%rax) # m_addr[9]=r15

        #restore rcx
        pop %rcx

        #call co_thread::yield_events_impl(int*, int*, int)
        #rdi,rsi,rdx,rcx 4 parameter stay no change
        push %rdi
        call _ZN9co_thread17yield_events_implEPiS0_i
        pop %rdi

        #restore_user_context
        mov 8(%rdi),%rax  # 把m_local_state的指针保存在rax
        mov 72(%rax),%r15 # r15=m_local_state[9]
        mov 64(%rax),%r14 # r14=m_local_state[8]
        mov 56(%rax),%r13 # r13=m_local_state[7]
        mov 48(%rax),%r12 # r12=m_local_state[6]
        mov 40(%rax),%rbx # rbx=m_local_state[5]
        mov 32(%rax),%rsi # rsi=m_local_state[4]
        mov 24(%rax),%rdi # rdi=m_local_state[3]
        mov 16(%rax),%rbp # rbp=m_local_state[2]
        mov 8(%rax),%rsp  # rsp=m_local_state[1]

        #set return value 2
        mov $2,%eax
        ret