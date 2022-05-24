#include <iostream>
#include "co_manager.h"
using namespace std;


void co_func1(){
    for(int i=0;i<10;++i){
        cout<<"from co_func2:"<<i<<enld;
        yield();
    }
}
void co_func2(){
    for(int i=0;i<10;++i){
        cout<<"from co_func2:"<<i<<enld;
        yield();
    }
}

int main(){
    co_manager env;
    env.add_task(co_func1);
    env.add_task(co_func2);
    env.run();
    return 0;
}