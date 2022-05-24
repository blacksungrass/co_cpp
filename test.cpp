
#include <iostream>
#include "co_manager.h"
using namespace std;

void func(co_thread& t){
    for(int i=0;i<10;++i){
        printf("%d\n",i);
        t.yield();
    }
}

int main(){
    co_thread t((void*)func);
    bool r = false;
    while(!r){
        r = t.run();
    }
    return 0;
}