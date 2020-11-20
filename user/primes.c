#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"



int main(int argc,char* argv[]){

    if(argc>1){
        fprintf(2,"errror: more argv\n");
        exit(1);
    }

    int buf[40];
    int cnt=0;
    buf[0]=34;
    for(int i=2;i<=35;i++){
       buf[++cnt]=i;
    }

    while(buf[0]>0){
        
        int p[2];
        
        int ret=pipe(p);
        if(ret==-1){
            fprintf(2,"pipe error\n");
            exit(1);
        }


        int pid=fork();
        if(pid<0){
            fprintf(2,"fork error\n");
            exit(1);
        }
        if(pid==0){
            //child
            close(p[1]);//close the write
            int * te=buf;
            int tep=0;
            while((tep=read(p[0],te,sizeof(buf)))==sizeof(buf)){
               te+=tep/sizeof(int);
            }
            
            printf("prime %d\n",buf[1]);
            int k=1;
            int pri=buf[1];
            for(int i=2;i<=buf[0];i++){
                if(buf[i]%pri){
                    buf[k++]=buf[i];
                } 
            }
            buf[0]=k-1;
        }
        else{
            //parent
            
            close(p[0]);//close the read
            if(write(p[1],buf,sizeof(buf))<sizeof(buf)){
                fprintf(2,"write error\n");
                close(p[1]);
                exit(1);
            }
            
            close(p[1]);
            wait(&pid);
            exit(0);
        }
    }
    exit(0);
    

}
