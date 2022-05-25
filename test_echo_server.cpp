#include <iostream>
#include "echo_server.cpp"
using namespace std;

int main(){
    echo_server server(7777,0);
    server.run();
}