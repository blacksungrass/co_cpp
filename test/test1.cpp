#include "co_env.h"
#include <vector>
using namespace std;

vector<int> v;

void func1(co_thread& th,void* args){
    for(int i=0;i<10;++i){
        v.emplace_back(i);
        th.yield();
    }
}

void func2(co_thread& th,void* args){
    for(int i=10;i<20;++i){
        v.emplace_back(i);
        th.yield();
    }
}

int main(){
    co_thread t1(func1,nullptr);
    co_thread t2(func2,nullptr);
    int r1 = 0,r2=0;
    while(r1!=1||r2!=1){
        if(r1!=1)
            r1 = t1.run();
        if(r2!=1)
            r2 = t2.run();
    }
    if(v.size()!=20)
        return 1;//error
    for(int i=0;i<20;i+=2){
        if(v[i]!=i/2){
            return 1;//error
        }
    }
    for(int i=1;i<20;i+=2){
        if(v[i]!=i/2+10){
            return 1;//error
        }
    }
    return 0;//test success
}