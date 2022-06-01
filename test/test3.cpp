/*
test timeout parameter of connection class
*/
#include "co_env.h"
#include "connection.h"
#include "co_thread.h"
#include <thread>
#include <unistd.h>
#include <sys/time.h>
using namespace std;


static void sleepms(int ms){
    struct timespec tm{};
    tm.tv_sec = ms/1000;
    ms %= 1000;
    tm.tv_nsec = ms*1000*1000;
    while(tm.tv_sec>0||tm.tv_nsec>0){
        int ret = nanosleep(&tm,&tm);
        if(ret==0)
            return;
    }
}

vector<int> v;

void func1(co_thread& t,void* args){
    int fd = *(int*)args;
    connection conn(fd,t);
    int data;
    while(true){
        int retval = conn.read_n_bytes(sizeof(int),(char*)&data,200);
        if(retval<0)
            break;
        v.emplace_back(data);
    }
}

void func2(int fd){
    for(int i=0;i<10;i++){
        write(fd,&i,sizeof(int));
        if(i==5){
            sleepms(300);
        }
        else{
            sleepms(100);
        }
    }
}

int main(){
    int fds[2];
    pipe(fds);
    co_env env;
    env.add_task(func1,(void*)(fds));
    thread t(func2,fds[1]);
    env.loop();
    t.join();
    if(v.size()!=6){
        return -1;
    }
    for(int i=0;i<=5;i++){
        if(v[i]!=i){
            return -1;
        }
    }
    return 0;
}