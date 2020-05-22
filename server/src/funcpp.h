#ifndef __FUNCPP_H__
#define __FUNCPP_H__

#include<iostream>
#include<string>
#include<algorithm>
#include<vector>
#include<deque>
#include<list>
#include<set>
#include<map>
#include<stack>
#include<queue>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<unistd.h>
#include<sys/types.h>
#include<dirent.h>
#include<time.h>
#include<sys/time.h>
#include<pwd.h>
#include<grp.h>
#include<fcntl.h>
#include<sys/select.h>
#include<sys/time.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/msg.h>
#include<signal.h>
#include<sys/msg.h>
#include<pthread.h>
#include<semaphore.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<sys/epoll.h>
#include<sys/uio.h>
#include<sys/mman.h>
#include<mysql/mysql.h>
#include<openssl/md5.h>
using namespace std;

#define MaxNum 100
#define ThreadNum 5
#define STR_LEN 10
#define VirtualPath "../virtualFile"
#define TimeOut 30
#define judgeArgc(argc,num) {if(argc!=num){printf("arg err!\n");return -1;}}

int tcpInit(char*,char*);
int send_n(int,void*,int);
int recv_n(int,void*,int);
int transFile(int,char*,long);//循环发送文件
int recvFile(int,char*,long);//循环接收文件
void GenerateStr(char*);//随机生成盐值

int sqlSingleSelect(char*,char*);//单返回值查询
int sqlFindData(char*);//返回是否有匹配
int sqlTableChange(char*);//插入、删除
#endif
