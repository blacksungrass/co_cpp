
#include <iostream>
#include "co_env.h"
using namespace std;

void func1(co_thread& t){
    for(int i=1;i<10;i+=2){
        cout<<"func1 i="<<i<<endl;
        //printf("func1 i=%d\n",i);
        t.yield();
    }
}

void func2(co_thread& t){
    for(int i=2;i<10;i+=2){
        cout<<"func2 i="<<i<<endl;
        //printf("func2 i=%d\n",i);
        t.yield();
    }
}

int main(){
    co_thread t1((void*)func1);
    co_thread t2((void*)func2);
    bool r = false;
    while(!r){
        bool r1 = t1.run();
        bool r2 = t2.run();
        r = r1&r2;
    }
    return 0;
}