#include <iostream>
#include "echo_server.h"
using namespace std;

int main(){
    echo_server server(7776,5);
    server.run();
}