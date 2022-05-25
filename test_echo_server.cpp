#include <iostream>
#include "echo_server.cpp"
using namespace std;

int main(){
    echo_server server(7776,5);
    server.run();
}