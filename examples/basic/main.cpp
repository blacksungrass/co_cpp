#include <iostream>
#include <pthread.h>
#include <thread>
#include "co_env.h"
using namespace std;

void co_func(co_thread& t,void* arg){
    long v = (long)arg;
    //printf("before v=%d\n",v);
    t.yield_for(v*1000);
    printf("v=%d\n",v);
}

void co_func1(co_thread& t,void* arg){
    for(int i=0;i<10;++i){
        printf("in co_func1 i=%d\n",i);
        t.yield_for(500);
    }
}
void co_func2(co_thread& t,void* arg){
    for(int i=0;i<10;++i){
        printf("in co_func2 i=%d\n",i);
        t.yield_for(500);
    }
}

int main(){
    
    co_env env;
    env.add_task(co_func1,nullptr);
    env.add_task(co_func2,nullptr);

    env.loop();

    for(long i=0;i<=20;++i){
        env.add_task(co_func,(void*)i);
    }
    env.loop();
    
    return 0;
}