#include "co_env.h"
#include "connection.h"
#include <iostream>
#include <fcntl.h>
using namespace std;

int fds[2];
vector<int> res;

void make_non_blocking(int fd){
    int old = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,old|O_NONBLOCK);
}

void func1(co_thread& t,void* args){
    int fd = ((int*)args)[0];
    connection conn(fd,t);
    while(true){
        int r;
        int cnt = conn.read_n_bytes(sizeof(int),(char*)&r,0);
        if(cnt<0)
            break;
        cout<<"i="<<r<<endl;
        res.emplace_back(r);
    }
    close(fd);
}

//write to fd(args)
void func2(co_thread& t,void* args){
    int fd = ((int*)args)[1];
    connection conn(fd,t);
    for(int i=0;i<10;++i){
        t.yield_event(fd,EPOLLOUT);
        conn.write((char*)&i,sizeof(int),0);
    }
    close(fd);
}


int main(){
    pipe(fds);
    make_non_blocking(fds[0]);
    make_non_blocking(fds[1]);
    co_env env;
    env.add_task(func1,(void*)fds);
    env.add_task(func2,(void*)fds);
    env.loop();
    if(res.size()!=10){
        return 1;//error
    }
    for(int i=0;i<10;++i){
        if(res[i]!=i){
            return 1;//error
        }
    }
    return 0;//test pass
}