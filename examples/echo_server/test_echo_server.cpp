#include <iostream>
#include "echo_server.h"
using namespace std;

void print_help_msg(const char* name){
    printf("usage:%s port timeout(in seconds)\n",name);
    printf("example:\n\"%s 7777 5\" means start a echo server on port 7777 and have an timeout of 5 seconds\n",name);
}

int main(int argc,char** argv){
    if(argc!=3){
        print_help_msg(argv[0]);
        return -1;
    }
    int port = atoi(argv[1]);
    int timeout = atoi(argv[2]);
    echo_server server(port,timeout);
    server.run();
}