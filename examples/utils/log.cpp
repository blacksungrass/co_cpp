#include "log.h"

void log(const char* filename,int line,const char* fmt){
    printf("%s:%d:%s\n",filename,line,fmt);
}