#pragma once

#include <stdio.h>

#define Log(x) log(__FILE__,__LINE__,x)

void log(const char* filename,int line,const char* fmt){
    printf("%s:%d:%s\n",filename,line,fmt);
}