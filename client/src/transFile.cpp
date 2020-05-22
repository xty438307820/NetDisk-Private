#include "funcpp.h"

int send_n(int newfd,void *data,int dataLen){
    int ret,total=0;
    char* snd=(char*)data;
    while(total<dataLen){
        ret=send(newfd,snd+total,dataLen-total,MSG_NOSIGNAL);//不被SIGPIPE中断
        if(-1==ret){
            return -1;
        }
        total+=ret;
    }
    return 0;
}

int recv_n(int newfd,void *data,int dataLen){
    int ret,total=0;
    char *rcv=(char*)data;
    while(total<dataLen){
        ret=recv(newfd,rcv+total,dataLen-total,0);
        if(0==ret||-1==ret){
            return -1;
        }
        total+=ret;
    }
    return 0;
}

int transFile(int newfd,char* fileName,long offset){

    //传文件名
    char buf[1024]={0};
    int dataLen;
    dataLen=strlen(fileName);
    int ret=send_n(newfd,&dataLen,sizeof(int));
    if(-1==ret){
        return -1;
    }
    ret=send_n(newfd,fileName,dataLen);
    if(-1==ret){
        return -1;
    }

    //传文件大小
    int fd=open(fileName,O_RDONLY);
    struct stat fileStat;
    fstat(fd,&fileStat);
    long totalSize=fileStat.st_size;
    dataLen=sizeof(long);
    ret=send_n(newfd,&dataLen,sizeof(int));
    if(-1==ret){
        close(fd);
        return -1;
    }
    ret=send_n(newfd,&totalSize,dataLen);
    if(-1==ret){
        close(fd);
        return -1;
    }

    long currentSize=offset,preSize=offset;
    long distSize=totalSize/1000;//每0.1%更新进度条

    //传文件内容
    //小于100M普通循环发送
    if(totalSize<=(long)100*1024*1024){
        lseek(fd,currentSize,SEEK_SET);
        while(dataLen=read(fd,buf,sizeof(buf))){
            ret=send_n(newfd,&dataLen,sizeof(int));
            if(-1==ret){
                close(fd);
                return -1;
            }
            ret=send_n(newfd,buf,dataLen);
            if(-1==ret){
                close(fd);
                return -1;
            }
            currentSize+=dataLen;
            if(currentSize-preSize>=distSize){
                //printf("%5.2f%s\r",(double)currentSize/totalSize*100,"%");
                //fflush(stdout);
                preSize=currentSize;
            }
        }
        ret=send_n(newfd,&dataLen,sizeof(int));
        if(-1==ret){
            close(fd);
            return -1;
        }
    }
    else{//大于100M用mmap发送
        char *p=(char*)mmap(NULL,totalSize,PROT_READ,MAP_SHARED,fd,0);
        dataLen=1024;
        while(currentSize<totalSize){
            if(totalSize-currentSize<dataLen) dataLen=totalSize-currentSize;
            ret=send_n(newfd,&dataLen,sizeof(int));
            if(-1==ret){
                munmap(p,totalSize);
                close(fd);
                return -1;
            }
            ret=send_n(newfd,p+currentSize,dataLen);
            if(-1==ret){
                munmap(p,totalSize);
                close(fd);
                return -1;
            }
            currentSize+=dataLen;
            if(currentSize-preSize>=distSize){
                //printf("%5.2f%s\r",(double)currentSize/totalSize*100,"%");
                //fflush(stdout);
                preSize=currentSize;
            }
        }
        munmap(p,totalSize);
    }
    //printf("100.0%s\n","%");
    close(fd);
    return 0;
}

int recvFile(int newfd,char* curPath,long offset){

    //接文件名
    char buf[1024]={0};
    int dataLen;
    int ret=recv_n(newfd,&dataLen,sizeof(int));
    if(-1==ret){
        return -1;
    }
    recv_n(newfd,buf+strlen(buf),dataLen);
    strcpy(buf,curPath);
    if(-1==ret){
        return -1;
    }

    //接文件大小
    int fd=open(buf,O_RDWR|O_CREAT|O_APPEND,0666);
    long totalSize;
    ret=recv_n(newfd,&dataLen,sizeof(int));
    if(-1==ret){
        close(fd);
        return -1;
    }
    ret=recv_n(newfd,&totalSize,dataLen);
    if(-1==ret){
        close(fd);
        return -1;
    }

    long currentSize=offset,preSize=offset;
    long distSize=totalSize/1000;//每0.1%更新进度条

    //接文件内容
    //小于100M普通循环接收
    if(totalSize<=(long)100*1024*1024){
        while(1){
            ret=recv_n(newfd,&dataLen,sizeof(int));
            if(0==dataLen) break;
            if(-1==ret){
                close(fd);
                return -1;
            }
            ret=recv_n(newfd,buf,dataLen);
            if(-1==ret){
                close(fd);
                return -1;
            }
            write(fd,buf,dataLen);
            currentSize+=dataLen;
            if(currentSize-preSize>=distSize){
                //printf("%5.2f%s\r",(double)currentSize/totalSize*100,"%");
                //fflush(stdout);
                preSize=currentSize;
            }
        }
    }
    else{//大于100M用mmap接收
        ftruncate(fd,totalSize);
        char *p=(char*)mmap(NULL,totalSize,PROT_WRITE,MAP_SHARED,fd,0);
        while(currentSize<totalSize){
            ret=recv_n(newfd,&dataLen,sizeof(int));
            if(-1==ret){
                munmap(p,totalSize);
                ftruncate(fd,currentSize);
                close(fd);
                return -1;
            }
            ret=recv_n(newfd,p+currentSize,dataLen);
            if(-1==ret){
                munmap(p,totalSize);
                ftruncate(fd,currentSize);
                close(fd);
                return -1;
            }
            currentSize+=dataLen;
            if(currentSize-preSize>=distSize){
                //printf("%5.2f%s\r",(double)currentSize/totalSize*100,"%");
                //fflush(stdout);
                preSize=currentSize;
            }
        }
        munmap(p,totalSize);
    }
    //printf("100.00%s\n","%");
    close(fd);
    return 0;
}
