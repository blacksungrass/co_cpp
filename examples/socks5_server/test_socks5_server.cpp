#include <iostream>
#include "socks5_server.h"
using namespace std;

void print_help_msg(const char* name){
    printf("usage:%s port \n",name);
    printf("example:\n\"%s 7777 5\" means start a socks5 server on port 7777\n",name);
}

int main(int argc,char** argv){
    if(argc!=2){
        print_help_msg(argv[0]);
        return -1;
    }
    int port = atoi(argv[1]);
    socks5_server server(port);
    server.run();
    return 0;
}