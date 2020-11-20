#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc,char* argv[]){
    
    int i;
    if(argc<=1){
        fprintf(2,"lack enough argment\n");
        exit(1);
    }
    if(argc<=2){
        i=atoi(argv[1]);
        sleep(i);
        exit(0);
    }
    fprintf(2,"too many argments\n");
    exit(1);
}
