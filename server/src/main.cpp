#include "thread_factory.h"

void ServerInit(char*,char*,char*,char*);//读配置文件
//四个参数为conf,ip,port,path,后三个为传出参数
void* supervise(void*);//监督子线程
void* threadFunc(void*);//子线程操作
void splitChar(char*,char*,char*);//分割char*,将中间空格分开,后两个参数为传出参数
int fileExist(char*,char*,char);//根据传入的绝对路径判断文件或文件夹是否存在,用于cd和gets,存在返回1,否则返回0
int userLogin(int,void*);//登录函数
int userRegister(int,void*);//注册函数
int userAction(void*);//登录成功,为用户服务
void* threadfunc(void*);//子线程puts,gets函数

int logfd;//日志记录fd
set<int> fdSet[TimeOut+1];//环形队列,用于30s断开连接,可改funcpp.h里面的TimeOut宏更改超时秒数
int fdPointer=0;//记录环形队列指针位置
pthread_mutex_t setMutex;
map<int,pNode> mp;//将newfd和pNode对应
map<string,int> tok;//将token与newfd对应
factory f;//全局变量f,用于puts和gets

int main(int argc,char* argv[])
{
    judgeArgc(argc,2);
    
    //读配置文件
    char ip[20]={0};
    char port[10]={0};
    char absPath[64]={0};
    ServerInit(argv[1],ip,port,absPath);

    //创建子线程
    int i,j;
    memset(&f,0,sizeof(f));
    f.pthid=(pthread_t*)calloc(ThreadNum+1,sizeof(pthread_t));
    f.threadQue=(pQue)calloc(1,sizeof(Que));
    f.threadQue->Capacity=MaxNum;
    pthread_mutex_init(&f.threadQue->queMutex,NULL);
    pthread_mutex_init(&setMutex,NULL);
    pthread_cond_init(&f.cond,NULL);
    f.state=1;//1为线程池开启
    logfd=open("../log/log",O_WRONLY|O_APPEND|O_CREAT,0666);
    for(i=0;i<ThreadNum;i++){
        pthread_create(&f.pthid[i],NULL,threadfunc,&f);
    }
    cout<<"thread pool start success"<<endl;
    
    //tcp初始化
    int socketfd=tcpInit(ip,port);
    listen(socketfd,MaxNum);

    //初始化epoll
    int epfd;//epollfd
    struct epoll_event event;
    epfd=epoll_create(1);
    struct epoll_event evs[2*MaxNum+1];
    memset(&event,0,sizeof(event));
    event.events=EPOLLIN;
    event.data.fd=socketfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,socketfd,&event);
    int waitNum,newfd,ret;
    pNode pnode;
    struct sockaddr_in sock;
    memset(&sock,0,sizeof(sockaddr_in));
    socklen_t addrlen=sizeof(sock);
    char buf[128]={0},bufTim[64]={0};
    time_t tim;
    int dataLen;
    int currentSize=0;//连上的客户端数目

    pthread_create(&f.pthid[ThreadNum],NULL,supervise,&epfd);

    while(1){
        waitNum=epoll_wait(epfd,evs,MaxNum+1,-1);
        for(i=0;i<waitNum;i++){
            //如果描述符是socketfd
            if(evs[i].data.fd==socketfd){
                newfd=accept(socketfd,(struct sockaddr*)&sock,&addrlen);
                printf("client %s %d connect\n",inet_ntoa(sock.sin_addr),ntohs(sock.sin_port));
                tim=time(NULL);
                ctime_r(&tim,bufTim);
                sprintf(buf,"client %s %d connect %s",inet_ntoa(sock.sin_addr),ntohs(sock.sin_port),bufTim);
                write(logfd,buf,strlen(buf));
                pnode=(pNode)calloc(1,sizeof(Node));
                pnode->newfd=newfd;
                pnode->pUserInfo=(pUserCtl)calloc(1,sizeof(UserCtl));
                strcpy(pnode->pUserInfo->absPath,absPath);
                strcpy(pnode->pUserInfo->Virtual,VirtualPath);
                if(currentSize<MaxNum){
                    currentSize++;
                    ret=0;
                    mp[pnode->newfd]=pnode;
                }
                else ret=-1;
                if(-1==ret){//队满直接关闭
                    close(newfd);
                    free(pnode);
                    pnode=NULL;
                }
                if(0==ret){
                    event.data.fd=newfd;
                    epoll_ctl(epfd,EPOLL_CTL_ADD,newfd,&event);
                }
            }
            else{
                newfd=evs[i].data.fd;
                pnode=mp[newfd];

                //插入环形队列
                pthread_mutex_lock(&setMutex);
                pnode->setPos=fdPointer-1;
                if(-1==pnode->setPos) pnode->setPos=TimeOut;
                fdSet[pnode->setPos].insert(pnode->newfd);
                pthread_mutex_unlock(&setMutex);

                dataLen=1;
                recv_n(newfd,&dataLen,sizeof(int));
                if(1==dataLen){//1为瞬时命令

                    //更新环形队列
                    pthread_mutex_lock(&setMutex);
                    fdSet[pnode->setPos].erase(pnode->newfd);
                    pnode->setPos=fdPointer-1;
                    if(-1==pnode->setPos) pnode->setPos=TimeOut;
                    fdSet[pnode->setPos].insert(pnode->newfd);
                    pthread_mutex_unlock(&setMutex);

                    if(0==pnode->pUserInfo->state){//刚连接只能接收1或2
                        ret=recv_n(newfd,&dataLen,sizeof(int));
                        if(-1==ret){
                            close(newfd);
                            event.data.fd=newfd;
                            epoll_ctl(epfd,EPOLL_CTL_DEL,newfd,&event);
                            free(mp[newfd]);
                            pnode=NULL;
                            mp.erase(newfd);
                            currentSize--;
                            continue;
                        }
                        if(1==dataLen) pnode->pUserInfo->state=1;//登录状态
                        else if(2==dataLen) pnode->pUserInfo->state=2;//注册状态
                    }
                    else if(1==pnode->pUserInfo->state){//登录
                        ret=userLogin(newfd,pnode);
                        if(-1==ret){//两次登录失败
                            close(newfd);
                            event.data.fd=newfd;
                            epoll_ctl(epfd,EPOLL_CTL_DEL,newfd,&event);
                            free(mp[newfd]);
                            pnode=NULL;
                            mp.erase(newfd);
                            currentSize--;
                        }
                    }
                    else if(2==pnode->pUserInfo->state){//注册
                        ret=userRegister(newfd,pnode);
                        if(-1==ret){//客户端断开,注册失败
                            close(newfd);
                            event.data.fd=newfd;
                            epoll_ctl(epfd,EPOLL_CTL_DEL,newfd,&event);
                            free(mp[newfd]);
                            pnode=NULL;
                            mp.erase(newfd);
                            currentSize--;
                        }
                    }
                    else if(3==pnode->pUserInfo->state){//登录成功
                        ret=userAction(pnode);
                        if(-1==ret){
                            cout<<"Client bye"<<endl;
                            tok.erase(pnode->pUserInfo->token);//注销token
                            close(newfd);
                            event.data.fd=newfd;
                            epoll_ctl(epfd,EPOLL_CTL_DEL,newfd,&event);
                            free(mp[newfd]);
                            pnode=NULL;
                            mp.erase(newfd);
                            currentSize--;
                        }
                    }
                }
                else if(2==dataLen){//2为puts或gets数据连接
                    
                    //puts和gets从环形队列删除
                    pthread_mutex_lock(&setMutex);
                    fdSet[pnode->setPos].erase(pnode->newfd);
                    pthread_mutex_unlock(&setMutex);

                    //执行token认证
                    recv_n(newfd,&dataLen,sizeof(int));
                    memset(buf,0,sizeof(buf));
                    recv_n(newfd,buf,dataLen);
                    //认证成功
                    if(tok[buf]&&strcmp(buf,mp[tok[buf]]->pUserInfo->token)==0){
                        ret=0;
                        send_n(newfd,&ret,sizeof(int));
                        strcpy(pnode->pUserInfo->absPath,mp[tok[buf]]->pUserInfo->absPath);
                        strcpy(pnode->pUserInfo->curPath,mp[tok[buf]]->pUserInfo->curPath);
                        strcpy(pnode->pUserInfo->UserName,pnode->pUserInfo->UserName);
                        pthread_mutex_lock(&f.threadQue->queMutex);
                        ret=queInsert(f.threadQue,pnode);
                        pthread_mutex_unlock(&f.threadQue->queMutex);
                        if(-1==ret){//队满
                            send_n(newfd,&ret,sizeof(int));
                            close(newfd);
                            free(mp[newfd]);
                            pnode=NULL;
                            mp.erase(newfd);
                            currentSize--;
                            continue;
                        }
                        event.data.fd=newfd;
                        epoll_ctl(epfd,EPOLL_CTL_DEL,newfd,&event);
                        send_n(newfd,&ret,sizeof(int));
                        pthread_cond_signal(&f.cond);
                        currentSize--;
                    }
                    else{//认证失败
                        ret=-1;
                        send_n(newfd,&ret,sizeof(int));
                        close(newfd);
                        event.data.fd=newfd;
                        epoll_ctl(epfd,EPOLL_CTL_DEL,newfd,&event);
                        free(mp[newfd]);
                        pnode=NULL;
                        mp.erase(newfd);
                        currentSize--;
                    }   
                }
            }
        }
    }

    close(epfd);
    close(logfd);
    close(socketfd);
    return 0;
}

int userAction(void* p){//登录成功,为用户服务
    char buf[128]={0};
    char buf1[128]={0};
    char operate[32]={0};//操作类型
    char operand[96]={0};//操作对象
    char *secret,*pwd;
    char sql[216]={0};
    pNode pnode=(pNode)p;
    int ret,i,dataLen,fd;

    DIR *dir;
    struct dirent *dnt;
    struct stat sta;
    long fileSize;

    time_t tim;
    tim=time(NULL);

    ret=recv_n(pnode->newfd,&dataLen,sizeof(int));
    if(-1==ret) return -1;
    ret=recv_n(pnode->newfd,buf,dataLen);
    if(-1==ret) return -1;

    //将操作数和对象分开
    splitChar(buf,operate,operand);

    if(strcmp(operate,"cd")==0){
        ctime_r(&tim,buf1);
        sprintf(buf,"%s: %s %s %s",pnode->pUserInfo->UserName,operate,operand,buf1);
        write(logfd,buf,strlen(buf));
        if(strcmp(operand,".")==0){
            ret=1;
            send_n(pnode->newfd,&ret,sizeof(int));
            return 0;
        }
        else if(strcmp(operand,"..")==0){
            if(strlen(pnode->pUserInfo->curPath)==0){
                ret=1;
                send_n(pnode->newfd,&ret,sizeof(int));
                return 0;
            }
            i=strlen(pnode->pUserInfo->curPath)-1;
            while(i>=0&&pnode->pUserInfo->curPath[i]!='/') i--;
            if(i>=0) pnode->pUserInfo->curPath[i]=0;
        }
        else if(operand[0]=='/'){
            strcat(pnode->pUserInfo->curPath,operand);
        }
        else{
            pnode->pUserInfo->curPath[strlen(pnode->pUserInfo->curPath)+1]=0;
            strcpy(buf,pnode->pUserInfo->absPath);
            strcat(buf,pnode->pUserInfo->curPath);
            ret=fileExist(buf,operand,'d');
            if(0==ret){
                send_n(pnode->newfd,&ret,sizeof(int));
                return 0;
            }
            pnode->pUserInfo->curPath[strlen(pnode->pUserInfo->curPath)]='/';
            strcat(pnode->pUserInfo->curPath,operand);
        }
        ret=1;
        send_n(pnode->newfd,&ret,sizeof(int));
    }
    else if(strcmp(operate,"ls")==0){
        ctime_r(&tim,buf1);
        sprintf(buf,"%s: %s %s",pnode->pUserInfo->UserName,operate,buf1);
        write(logfd,buf,strlen(buf));
        strcpy(buf,pnode->pUserInfo->absPath);
        strcat(buf,pnode->pUserInfo->curPath);
        dir=opendir(buf);
        i=strlen(buf)+1;
        buf[i-1]='/';

        while((dnt=readdir(dir))!=NULL){
            if(dnt->d_name[0]=='.'||!strcmp("..",dnt->d_name)||!strcmp(".",dnt->d_name)) continue;
            buf[i]=0;
            strcat(buf,dnt->d_name);
            stat(buf,&sta);

            //传文件类型
            ret=sta.st_mode>>12;
            if(8==ret){//8为文件
                buf1[0]='-';
                send_n(pnode->newfd,buf1,1);
            }
            else if(4==ret){//4为目录
                buf1[0]='d';
                send_n(pnode->newfd,buf1,1);
            }

            //传文件名
            strcpy(buf1,dnt->d_name);
            dataLen=strlen(buf1);
            send_n(pnode->newfd,&dataLen,sizeof(int));
            send_n(pnode->newfd,buf1,dataLen);

            //传文件大小
            send_n(pnode->newfd,&sta.st_size,sizeof(long));
        }
        dataLen=0;
        send_n(pnode->newfd,&dataLen,1);
        closedir(dir);
        dir=NULL;
    }
    else if(strcmp(operate,"puts")==0){
        ctime_r(&tim,buf1);
        sprintf(buf,"%s: %s %s %s",pnode->pUserInfo->UserName,operate,operand,buf1);
        write(logfd,buf,strlen(buf));
        strcpy(buf,pnode->pUserInfo->absPath);
        strcat(buf,pnode->pUserInfo->curPath);

        dataLen=recv_n(pnode->newfd,&ret,sizeof(int));
        if(-1==ret) return 0;
        ret=fileExist(buf,operand,'d');
        if(1==ret){//存在同名文件夹,不能puts
            ret=-1;
            send_n(pnode->newfd,&ret,sizeof(int));
            return 0;
        }
        send_n(pnode->newfd,&ret,sizeof(int));

    }
    else if(strcmp(operate,"gets")==0){
        ctime_r(&tim,buf1);
        sprintf(buf,"%s: %s %s %s",pnode->pUserInfo->UserName,operate,operand,buf1);
        write(logfd,buf,strlen(buf));
        strcpy(buf,pnode->pUserInfo->absPath);
        strcat(buf,pnode->pUserInfo->curPath);
        ret=fileExist(buf,operand,'-');
        if(0==ret){//文件不存在,gets失败
            ret=-1;
            send_n(pnode->newfd,&ret,sizeof(int));
            return 0;
        }
        else if(1==ret){//文件存在
            send_n(pnode->newfd,&ret,sizeof(int));
        }
    }
    else if(strcmp(operate,"remove")==0){
        ctime_r(&tim,buf1);
        sprintf(buf,"%s: %s %s %s",pnode->pUserInfo->UserName,operate,operand,buf1);
        write(logfd,buf,strlen(buf));
        strcpy(buf,pnode->pUserInfo->absPath);
        strcat(buf,pnode->pUserInfo->curPath);
        ret=fileExist(buf,operand,'d');
        if(1==ret){//删除文件夹
            send_n(pnode->newfd,&ret,sizeof(int));
            buf[strlen(buf)+1]=0;
            buf[strlen(buf)]='/';
            strcat(buf,operand);
            remove(buf);
            return 0;
        }
        ret=fileExist(buf,operand,'-');
        if(1==ret){//文件存在
            send_n(pnode->newfd,&ret,sizeof(int));
            buf[strlen(buf)+1]=0;
            buf[strlen(buf)]='/';
            strcat(buf,operand);
            stat(buf,&sta);
            if(sta.st_nlink>2){
                remove(buf);
            }
            else{//硬链接数=2,删除用户目录文件、虚拟目录文件
                fd=open(buf,O_RDONLY);
                pwd=(char*)mmap(NULL,sta.st_size,PROT_READ,MAP_SHARED,fd,0);
                secret=(char*)MD5((unsigned char*)pwd,sta.st_size,NULL);
                munmap(pwd,sta.st_size);
                close(fd);

                strcpy(buf1,VirtualPath);
                buf1[strlen(buf1)+1]=0;
                buf1[strlen(buf1)]='/';
                strcat(buf1,secret);
                sprintf(sql,"delete from FileInfo where FileName='%s'",secret);
                sqlTableChange(sql);
                remove(buf);
                remove(buf1);
            }
            return 0;
        }
        else if(0==ret){//文件或文件夹不存在
            ret=-1;
            send_n(pnode->newfd,&ret,sizeof(int));
            return 0;
        }
    }
    else if(strcmp(operate,"pwd")==0){
        ctime_r(&tim,buf1);
        sprintf(buf,"%s: %s %s",pnode->pUserInfo->UserName,operate,buf1);
        write(logfd,buf,strlen(buf));
        memset(buf,0,sizeof(buf));
        strcpy(buf,pnode->pUserInfo->curPath);
        if(strlen(buf)==0){
            buf[0]='/';
        }
        dataLen=strlen(buf);
        send_n(pnode->newfd,&dataLen,sizeof(int));
        send_n(pnode->newfd,buf,dataLen);
    }
    else if(strcmp(operate,"mkdir")==0){
        ctime_r(&tim,buf1);
        sprintf(buf,"%s: %s %s",pnode->pUserInfo->UserName,operate,buf1);
        write(logfd,buf,strlen(buf));
        strcpy(buf,pnode->pUserInfo->absPath);
        strcat(buf,pnode->pUserInfo->curPath);
        ret=fileExist(buf,operand,'d');
        if(1==ret) ret=-1;
        if(0==ret) ret=fileExist(buf,operand,'-');
        if(1==ret) ret=-1;
        send_n(pnode->newfd,&ret,sizeof(int));
        if(0==ret){
            buf[strlen(buf)+1]=0;
            buf[strlen(buf)]='/';
            strcat(buf,operand);
            mkdir(buf,0775);
        }
    }
    return 0;
}

void* threadfunc(void* p){//puts和gets函数
    pNode pnode;
    char buf[128]={0};
    char buf1[128]={0};
    char operate[32]={0};//操作类型
    char operand[96]={0};//操作对象
    char sql[216]={0};
    int ret,dataLen,i;

    struct stat sta;
    long fileSize;

    while(1){
        memset(buf,0,128);
        memset(buf1,0,sizeof(buf1));
        memset(operate,0,sizeof(operate));
        memset(operand,0,sizeof(operand));

        pthread_mutex_lock(&f.threadQue->queMutex);
        if(0==f.threadQue->currentSize){
            pthread_cond_wait(&f.cond,&f.threadQue->queMutex);
        }
        ret=quePop(f.threadQue,&pnode);
        pthread_mutex_unlock(&f.threadQue->queMutex);
        if(-1==ret){
            continue;
        }

        //拿到结点,开始传数据
        //重新接收命令
        ret=recv_n(pnode->newfd,&dataLen,sizeof(int));
        if(-1==ret) goto end;
        ret=recv_n(pnode->newfd,buf,dataLen);
        if(-1==ret) goto end;
        splitChar(buf,operate,operand);

        if(strcmp(operate,"puts")==0){
            //查找数据库,看虚拟目录是否有相同文件
            recv_n(pnode->newfd,&dataLen,sizeof(int));
            recv_n(pnode->newfd,buf1,dataLen);//文件hash值
            sprintf(sql,"select FileName from FileInfo where FileName='%s'",buf1);
            ret=sqlFindData(sql);
            if(1==ret){//虚拟目录存在此文件,执行秒传
                ret=2;
                send_n(pnode->newfd,&ret,sizeof(int));
                strcpy(buf,pnode->pUserInfo->Virtual);
                buf[strlen(buf)+1]=0;
                buf[strlen(buf)]='/';
                strcat(buf,buf1);//buf是虚拟目录
                strcpy(buf1,pnode->pUserInfo->absPath);
                strcat(buf1,pnode->pUserInfo->curPath);
                buf1[strlen(buf1)+1]=0;
                buf1[strlen(buf1)]='/';
                strcat(buf1,operand);//buf1是用户目录
                link(buf,buf1);
                goto end;
            }
            strcpy(buf,pnode->pUserInfo->absPath);
            strcat(buf,pnode->pUserInfo->curPath);
            ret=fileExist(buf,operand,'-');
            if(1==ret){//文件存在,断点续传
                send_n(pnode->newfd,&ret,sizeof(int));
                i=strlen(buf);
                buf[strlen(buf)+1]=0;
                buf[strlen(buf)]='/';
                strcat(buf,operand);
                stat(buf,&sta);
                send_n(pnode->newfd,&sta.st_size,sizeof(long));
                fileSize=sta.st_size;
                buf[i]=0;
            }
            else if(0==ret){
                fileSize=0;
                send_n(pnode->newfd,&ret,sizeof(int));
            }
            ret=recvFile(pnode->newfd,buf,fileSize);
            if(0==ret){
                sprintf(sql,"insert into FileInfo values('%s')",buf1);
                strcpy(buf,pnode->pUserInfo->Virtual);
                buf[strlen(buf)+1]=0;
                buf[strlen(buf)]='/';
                strcat(buf,buf1);//buf为虚拟目录路径
                strcpy(buf1,pnode->pUserInfo->absPath);
                strcat(buf1,pnode->pUserInfo->curPath);
                buf1[strlen(buf1)+1]=0;
                buf1[strlen(buf1)]='/';
                strcat(buf1,operand);//buf1为用户空间目录路径
                link(buf1,buf);//建立硬链接
                ret=sqlTableChange(sql);
            }
            else if(-1==ret) perror("puts failed");

        }
        else if(strcmp(operate,"gets")==0){
            recv_n(pnode->newfd,&ret,sizeof(int));
            if(1==ret){//文件存在,进行断点续传
                recv_n(pnode->newfd,&fileSize,sizeof(int));
            }
            else{
                fileSize=0;
            }
            strcpy(buf,pnode->pUserInfo->absPath);
            strcat(buf,pnode->pUserInfo->curPath);
            buf[strlen(buf)+1]=0;
            buf[strlen(buf)]='/';
            strcat(buf,operand);
            ret=transFile(pnode->newfd,buf,fileSize);
            if(0==ret) cout<<"gets success"<<endl;
            else if(-1==ret) perror("gets failed");
        }

end:
        close(pnode->newfd);
        free(mp[pnode->newfd]);
        mp.erase(pnode->newfd);
        pnode=NULL;
    }
}

int userRegister(int newfd,void* p){
    pNode pnode=(pNode)p;
    char buf[128]={0};
    char sql[216]={0};
    char salt[32]={0};
    int dataLen,ret;
    char workNum='0';
    recv_n(newfd,&workNum,1);
    if('1'==workNum){//注册第一步,输入用户名
        recv_n(newfd,&dataLen,sizeof(int));
        recv_n(newfd,buf,dataLen);
        sprintf(sql,"select * from UserInfo where UserName='%s'",buf);
        ret=sqlFindData(sql);
        send_n(newfd,&ret,sizeof(int));
        if(ret==0){
            strcpy(pnode->pUserInfo->UserName,buf);
        }
        return 0;
    }
    else if('2'==workNum){//注册第二步,输入密码
        recv_n(newfd,&dataLen,sizeof(int));
        recv_n(newfd,&salt,dataLen);
        recv_n(newfd,&dataLen,sizeof(int));
        recv_n(newfd,buf,dataLen);

        sprintf(sql,"insert into UserInfo(UserName,Salt,PassWord) values('%s','%s','%s')",pnode->pUserInfo->UserName,salt,buf);
        sqlTableChange(sql);

        memset(buf,0,sizeof(buf));
        strcpy(buf,pnode->pUserInfo->absPath);
        buf[strlen(buf)]='/';
        strcat(buf,pnode->pUserInfo->UserName);
        mkdir(buf,0775);
        pnode->pUserInfo->state=0;//注册成功,返回选择操作界面
        return 0;
    }
    return -1;
}

int userLogin(int newfd,void* p){
    pNode pnode=(pNode)p;
    char buf[128]={0};
    char buf1[128]={0};
    char UserName[32]={0};
    char sql[216]={0};
    char salt[32]={0};
    int dataLen,ret;
    char workNum='0';
    time_t tim;
    recv_n(newfd,&workNum,1);
    //客户端登录第一步,输入用户名
    if('1'==workNum){
        recv_n(newfd,&dataLen,sizeof(int));
        recv_n(newfd,UserName,dataLen);
        sprintf(sql,"select Salt from UserInfo where UserName='%s'",UserName);
        sqlSingleSelect(sql,salt);
        if(strlen(salt)==0) strcpy(salt,"$6$sss");//没有该用户则给该盐值
        dataLen=strlen(salt);
        send_n(newfd,&dataLen,sizeof(int));
        send_n(newfd,salt,dataLen);
        strcpy(pnode->pUserInfo->UserName,UserName);
        return 0;
    }//客户端获取到盐值
    //客户端登录第二步,输入密码
    else if('2'==workNum){
        recv_n(newfd,&dataLen,sizeof(int));
        recv_n(newfd,buf,dataLen);
        sprintf(sql,"select PassWord from UserInfo where UserName='%s'",pnode->pUserInfo->UserName);
        sqlSingleSelect(sql,buf1);
        if(strcmp(buf,buf1)==0){//验证成功
            ret=1;
            send_n(newfd,&ret,sizeof(int));
            pnode->pUserInfo->absPath[strlen(pnode->pUserInfo->absPath)]='/';
            strcat(pnode->pUserInfo->absPath,pnode->pUserInfo->UserName);
            pnode->pUserInfo->state=3;//状态改为3登录成功

            //生成token值,传给客户端
            tim=time(NULL);
            sprintf(buf,"%ld%s",tim,pnode->pUserInfo->UserName);
            pnode->pUserInfo->token=(char*)MD5((unsigned char*)buf,strlen(buf),NULL);
            dataLen=strlen(pnode->pUserInfo->token);
            send_n(newfd,&dataLen,sizeof(int));
            send_n(newfd,pnode->pUserInfo->token,dataLen);
            tok[pnode->pUserInfo->token]=newfd;
        }
        else{//第一次登录失败
            ret=0;
            send_n(newfd,&ret,sizeof(int));
        }
        return 0;
    }
    //第二次登录
    else if('3'==workNum){
        recv_n(newfd,&dataLen,sizeof(int));
        recv_n(newfd,buf,dataLen);
        sprintf(sql,"select PassWord from UserInfo where UserName='%s'",pnode->pUserInfo->UserName);
        sqlSingleSelect(sql,buf1);
        if(strcmp(buf,buf1)==0){//登录成功
            ret=1;
            send_n(newfd,&ret,sizeof(int));
            pnode->pUserInfo->absPath[strlen(pnode->pUserInfo->absPath)]='/';
            strcat(pnode->pUserInfo->absPath,pnode->pUserInfo->UserName);
            pnode->pUserInfo->state=3;//状态改为3登录成功

            //生成token值,传给客户端
            tim=time(NULL);
            sprintf(buf,"%ld%s",tim,pnode->pUserInfo->UserName);
            pnode->pUserInfo->token=(char*)MD5((unsigned char*)buf,strlen(buf),NULL);
            dataLen=strlen(pnode->pUserInfo->token);
            send_n(newfd,&dataLen,sizeof(int));
            send_n(newfd,pnode->pUserInfo->token,dataLen);
            tok[pnode->pUserInfo->token]=newfd;
            return 0;
        }
        else{
            ret=0;
            send_n(pnode->newfd,&ret,sizeof(int));
            return -1;
        }
    }
    return -1;
}

//监督子线程
void* supervise(void *p){
    //每扫描一次将set里面有的fd关闭,表明30秒超时
    set<int>::iterator it;
    int epfd=*(int*)p;
    struct epoll_event event;
    memset(&event,0,sizeof(event));
    event.events=EPOLLIN;
    while(1){
        sleep(1);
        pthread_mutex_lock(&setMutex);
        for(it=fdSet[fdPointer].begin();it!=fdSet[fdPointer].end();it++){
            event.data.fd=*it;
            epoll_ctl(epfd,EPOLL_CTL_DEL,*it,&event);
            close(*it);
            free(mp[*it]);
            mp.erase(*it);
        }
        fdSet[fdPointer].clear();
        fdPointer=(fdPointer+1)%(TimeOut+1);
        pthread_mutex_unlock(&setMutex);
    }
}

//绝对路径如/home/adam，最后一个文件夹后面无'/'
int fileExist(char* absPath,char* fileName,char fileType){
    DIR *dir=opendir(absPath);
    struct dirent *dnt;
    char buf[128]={0};
    strcpy(buf,absPath);
    buf[strlen(buf)]='/';
    int i=strlen(buf);
    struct stat sta;
    char type;
    while((dnt=readdir(dir))!=NULL){
        buf[i]=0;
        strcat(buf,dnt->d_name);
        stat(buf,&sta);
        if(sta.st_mode>>12==4){//4是目录
            type='d';
        }
        else if(sta.st_mode>>12==8){//8是文件
            type='-';
        }
        if(strcmp(dnt->d_name,fileName)==0&&fileType==type){
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}

//将from中间的空格分开,前部分为操作数,后部分为操作对象
void splitChar(char* from,char* operate,char* operand){
    int fromLen=strlen(from);
    int i=0,j=0;
    while(from[i]==' '&&i<fromLen) i++;
    while(from[i]!=' '&&i<fromLen) operate[j++]=from[i++];
    while(from[i]==' '&&i<fromLen) i++;
    j=0;
    while(operate[i]!=' '&&i<fromLen) operand[j++]=from[i++];
}

void ServerInit(char* conf,char* ip,char* port,char* absPath){
    int fd=open(conf,O_RDONLY);
    char buf[128]={0};
    read(fd,buf,sizeof(buf));
    close(fd);
    int cnt=0;
    int start=0;
    while(buf[start+cnt]!='\n'){
        cnt++;
    }
    strncpy(ip,buf,cnt);
    start=start+cnt+1;
    cnt=0;
    while(buf[start+cnt]!='\n'){
        cnt++;
    }
    strncpy(port,buf+start,cnt);
    start=start+cnt+1;
    while(buf[start+cnt]!='\n'){
        cnt++;
    }
    strncpy(absPath,buf+start,cnt);
}
