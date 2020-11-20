#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char* argv[]){

    int p1[2];
    int p2[2];
    int ret1=pipe(p1);
    int ret2=pipe(p2);
    if(ret1==-1||ret2==-1){
        fprintf(2,"pipe error\n");
        exit(1);
    }

    int childpid=fork();
    if(childpid<0){
        fprintf(2,"fork error\n");
        exit(1);
    }
    else if(childpid==0){
        //child
        close(p1[1]);//close the write
        close(p2[0]);//close the read
        char temp='B';
        if(read(p1[0],&temp,sizeof(temp))>0){
            printf("%d: received ping\n",getpid());
            close(p1[0]);
        }
        else{
            close(p1[0]);
            fprintf(2,"read error\n");
            exit(1);
        }

        if(write(p2[1],&temp,sizeof(temp))<=0){
            fprintf(2,"write error\n");
            exit(1);
        }
        close(p2[1]);
        exit(0);
    }
    else {
        //parent
        close(p1[0]);//close the read
        close(p2[1]);//close the write
        char temp2='C';
        if(write(p1[1],&temp2,sizeof(temp2))<=0){
            fprintf(2,"write error\n");
            exit(1);
        }
        close(p1[0]);

        
        if(read(p2[0],&temp2,sizeof(temp2))>0){
            printf("%d: received pong\n",getpid());
            close(p2[0]);
            exit(0);
        }
        else{
            close(p2[0]);
            fprintf(2,"read error\n");
            exit(1);
        }
       
    }
    exit(0);
}
