#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

#define Maxn 512

int main(int argc, char* argv[]){
    
    char* argtemp[MAXARG];
    char stdoutput[Maxn];
    char * temp=stdoutput;
    if(argc<=1||argc > MAXARG){
        fprintf(2,"wrong args\n");
        exit(1);
    }
    for(int j=1;j<argc;j++){
        argtemp[j-1]=argv[j];
    }
    if(strcmp(argtemp[0],"-n")==0){
        for(int i=0;i<argc-1;i++){
            argtemp[i]=argtemp[i+2];
        }
        argc-=2;
        argtemp[argc-1]=0;
    }

    int n;
    while(1){

        for(int i=0;i<Maxn;i++){
            stdoutput[i]=0;
        }
        temp=stdoutput;
        while((n=read(0,temp,1))>0){
            temp+=n;
            if((*(temp-1))=='\n'){
                break;
            }
        }
        if(temp-stdoutput>Maxn){
            fprintf(2,"xargs: too long\n");
            exit(1);
        }
        if(n<0){
            fprintf(2,"something wrong with read\n");
            exit(1);
        }
        if(n==0){
            break;
            exit(0);
        }
        temp=stdoutput;

        int k=argc-1;

        while(*temp){
            switch (*temp){
                case ' ':
                    (*temp)=0;
                    k++;
                    temp++;
                    while((*temp)==' '){
                        temp++;
                    }
                    break;
                case '\n':
                    (*temp)=0;
                    temp++;
                    argtemp[k+1]=0;
                    break;
                default:
                    argtemp[k]=temp;
                    while((*temp)!=' '&&(*temp)!='\n'&&(*temp)){
                        temp++;
                    }
                    break;
            }
        }

        int pid=fork();
        if(pid==0){
            exec(argtemp[0], argtemp);
            exit(0);
        }
        else if(pid<0){
            fprintf(2,"xargs: fork fail\n");
            exit(2);
        }
        else{
            wait((int *)0);
        }
    
    }
    exit(0);
}