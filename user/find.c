#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void dir_re(char* path,char* file){
    int fd;
    char buf[512], *p;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    if(st.type==T_DIR){
        strcpy(buf, path);
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
            printf("find: path too long\n");
            return;
        }
        p = buf+strlen(buf);
    
        if(*(p-1)!='/'){
            *p++='/';
        }
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)
                continue;
            if(!strlen(de.name)||!strcmp(de.name,".")||!strcmp(de.name,".."))
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            int fdtemp;
            if((fdtemp = open(buf, 0)) < 0){
                fprintf(2, "find: cannot open %s\n", buf);
            }
            else if(fstat(fdtemp, &st) < 0){
                fprintf(2, "find: cannot stat %s\n",buf);
                close(fdtemp);
            }
            else{
                if(st.type==T_DIR){
                    dir_re(buf,file);
                }
                else{
                    if(!strcmp(de.name,file)){
                        printf("%s\n",buf);
                    }
                }
                close(fdtemp);
            }
        }
        *(--p)=0;
    }
    else{
        fprintf(2,"find: can't search into %s\n",path);
    }
    close(fd);
    return;
}

int main(int argc,char* argv[]){

    char path1[512];
    if(argc<2){
        fprintf(2,"lack some argment\n");
        exit(1);
    }
    if(argc==2){
        path1[0]='.';
        dir_re(path1,argv[1]);
        exit(0);
    }
    if(argc==3){
        memcpy(path1,argv[1],sizeof(argv[1]));
        dir_re(path1,argv[2]);
        exit(0);
    }
    else{
        fprintf(2,"too many argments\n");
        exit(1);
    }
}