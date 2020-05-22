#include "funcpp.h"

void splitChar(char*,char*,char*);//分割char*,将中间空格分开,后两个参数为传出参数
int fileExist(char*,char*,char);//根据传入的相对路径判断文件或文件夹是否存在
void* threadFunc(void*);//子线程执行puts和gets

char ip[32];
char port[10];
char token[32]={0};

int main(int argc,char* argv[])
{
    judgeArgc(argc,3);
    
    strcpy(ip,argv[1]);
    strcpy(port,argv[2]);
    //客户端连接
    int socketfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in sock;
    memset(&sock,0,sizeof(sockaddr_in));
    sock.sin_family=AF_INET;
    sock.sin_port=htons(atoi(argv[2]));
    sock.sin_addr.s_addr=inet_addr(argv[1]);
    int ret=connect(socketfd,(struct sockaddr*)&sock,sizeof(sockaddr_in));
    if(-1==ret){
        perror("connect");
        return -1;
    }
    cout<<"connect success"<<endl;

    char *action=(char*)calloc(128,sizeof(char));
    char buf[128]={0};
    char operate[32]={0};
    char operand[96]={0};
    char *pwd,*secret;
    char salt[32]={0};
    int dataLen,fd;
    long fileSize;
    struct stat sta;
    char workNum;
    pthread_t threadid;

begin:
    cout<<"Please choose your action:"<<endl;
    cout<<"1.login"<<endl;
    cout<<"2.register"<<endl;
    cin>>dataLen;
    while(!(dataLen==1||dataLen==2)){
        cout<<"Please enter 1 to login or 2 to register"<<endl;
        cin>>dataLen;
    }
    //瞬时命令先发送1
    ret=1;
    send_n(socketfd,&ret,sizeof(int));
    send_n(socketfd,&dataLen,sizeof(int));
    if(dataLen==1){//登录
        cout<<"Please input your UserName:";
        scanf("%s",buf);
        getchar();
        dataLen=strlen(buf);

        //登录第一步,编号1
        ret=1;//瞬时命令
        send_n(socketfd,&ret,sizeof(int));
        workNum='1';
        send_n(socketfd,&workNum,1);
        send_n(socketfd,&dataLen,sizeof(int));
        send_n(socketfd,buf,dataLen);
        memset(salt,0,sizeof(salt));
        recv_n(socketfd,&dataLen,sizeof(int));
        recv_n(socketfd,salt,dataLen);

        //登录第二步,编号2
        ret=1;//瞬时命令
        send_n(socketfd,&ret,sizeof(int));
        pwd=getpass("Input password:");
        secret=crypt(pwd,salt);
        dataLen=strlen(secret);
        workNum='2';
        send_n(socketfd,&workNum,1);
        send_n(socketfd,&dataLen,sizeof(int));
        send_n(socketfd,secret,dataLen);

        recv_n(socketfd,&ret,sizeof(int));
        if(1==ret){//第一次登录成功
            cout<<"login success"<<endl;
            //登录成功,获取token
            recv_n(socketfd,&dataLen,sizeof(int));
            recv_n(socketfd,token,dataLen);
        }
        else if(0==ret){
            pwd=getpass("Uncorrect username or password,input password again:");
            secret=crypt(pwd,salt);
            dataLen=strlen(secret);
            //登录第三步,第一次输入密码有误,第二次输入密码
            ret=1;
            send_n(socketfd,&ret,sizeof(int));
            workNum='3';
            send_n(socketfd,&workNum,1);
            send_n(socketfd,&dataLen,sizeof(int));
            send_n(socketfd,secret,dataLen);
            recv_n(socketfd,&ret,sizeof(int));
            if(1==ret){//第二次登录成功
                cout<<"login success"<<endl;
                //登录成功,获取token
                recv_n(socketfd,&dataLen,sizeof(int));
                recv_n(socketfd,token,dataLen);
            }
            else if(0==ret){
                cout<<"login fail"<<endl;
                return -1;
            }
        }
    }
    else{//注册

        //注册第一步,输入用户名
        cout<<"Please input you UserName:";
        scanf("%s",buf);
        dataLen=strlen(buf);
        while(dataLen>30){
            cout<<"UserName should be less than 30 bytes,enter anain:";
            memset(buf,0,sizeof(buf));
            scanf("%s",buf);
            dataLen=strlen(buf);
        }
        ret=1;
        send_n(socketfd,&ret,sizeof(int));
        workNum='1';
        send_n(socketfd,&workNum,1);
        send_n(socketfd,&dataLen,sizeof(int));
        send_n(socketfd,buf,dataLen);

        recv_n(socketfd,&ret,sizeof(int));
        while(ret){
            cout<<"UserName already exists,enter new UserName:";
            memset(buf,0,sizeof(buf));
            scanf("%s",buf);
            dataLen=strlen(buf);
            ret=1;
            send_n(socketfd,&ret,sizeof(int));
            send_n(socketfd,&workNum,1);
            send_n(socketfd,&dataLen,sizeof(int));
            send_n(socketfd,buf,dataLen);
            recv_n(socketfd,&ret,sizeof(int));
        }

        //注册第二步,输入密码
        pwd=getpass("Input password:");
        GenerateStr(salt);
        dataLen=strlen(salt);
        ret=1;
        send_n(socketfd,&ret,sizeof(int));
        workNum='2';
        send_n(socketfd,&workNum,1);
        send_n(socketfd,&dataLen,sizeof(int));
        send_n(socketfd,salt,dataLen);
        secret=crypt(pwd,salt);
        dataLen=strlen(secret);
        send_n(socketfd,&dataLen,sizeof(int));
        send_n(socketfd,secret,dataLen);
        cout<<"register success"<<endl;
        goto begin;
    }

    while(1){
        memset(buf,0,sizeof(buf));
        memset(operate,0,sizeof(operate));
        memset(operand,0,sizeof(operand));
        fgets(buf,sizeof(buf),stdin);
        buf[strlen(buf)-1]=0;
        if(strcmp(buf,"clear")==0){
            system("clear");
            continue;
        }
        //瞬时命令先发送1
        ret=1;
        send_n(socketfd,&ret,sizeof(int));

        dataLen=strlen(buf);
        ret=send_n(socketfd,&dataLen,sizeof(int));
        if(-1==ret){
            cout<<"Connection close"<<endl;
            break;
        }
        ret=send_n(socketfd,buf,dataLen);
        if(-1==ret){
            cout<<"Connection close"<<endl;
            break;
        }
        splitChar(buf,operate,operand);
        
        if(strcmp(operate,"cd")==0){
            dataLen=recv_n(socketfd,&ret,sizeof(int));
            if(-1==dataLen){
                cout<<"Connection close"<<endl;
                return -1;
            }
            if(0==ret) cout<<"No such file or directory"<<endl;
            continue;
        }
        else if(strcmp(operate,"ls")==0){
            printf("type                        name                 size\n");
            while(1){
                //获取文件类型,-为文件,d为目录
                ret=recv_n(socketfd,buf,1);
                if(-1==ret){
                    cout<<"Connection close"<<endl;
                    return -1;
                }
                if(buf[0]==0) break;
                printf("%c",buf[0]);

                //获取文件名
                recv_n(socketfd,&dataLen,sizeof(int));
                recv_n(socketfd,buf,dataLen);
                buf[dataLen]=0;
                printf(" %30s",buf);

                //获取文件大小
                recv_n(socketfd,&fileSize,sizeof(long));

                printf("%20ldB\n",fileSize);
            }

        }
        else if(strcmp(operate,"puts")==0){
            
            fd=open(operand,O_RDONLY);
            ret=send_n(socketfd,&fd,sizeof(int));
            if(-1==ret){
                cout<<"Connection close"<<endl;
                close(socketfd);
                continue;
            }
            if(-1==fd){
                cout<<"no this file"<<endl;
                continue;
            }
            recv_n(socketfd,&ret,sizeof(int));
            if(-1==ret){//存在相同名的文件夹,不能puts
                cout<<"There is a directory with same name.Puts failed"<<endl;
                continue;
            }
            //把命令传给子线程
            strcpy(action,buf);
            pthread_create(&threadid,NULL,threadFunc,action);
/*
            //------------------------------
            //token认证
            dataLen=strlen(token);
            ret=send_n(socketfd,&dataLen,sizeof(int));
            if(-1==ret){
                cout<<"Connection close"<<endl;
                return -1;
            }
            ret=send_n(socketfd,token,dataLen);
            if(-1==ret){
                cout<<"Connection close"<<endl;
                return -1;
            }
            recv_n(socketfd,&ret,sizeof(int));
            if(0==ret){//token认证失败
                cout<<"permissions deny"<<endl;
                continue;
            }

            fd=open(operand,O_RDONLY);
            ret=send_n(socketfd,&fd,sizeof(int));
            if(-1==ret){
                cout<<"Connection close"<<endl;
                return -1;
            }
            if(-1==fd){
                cout<<"no this file"<<endl;
                continue;
            }
            dataLen=strlen(operand);
            ret=send_n(socketfd,&dataLen,sizeof(int));
            if(-1==ret){
                cout<<"Connection close"<<endl;
                return -1;
            }
            send_n(socketfd,operand,dataLen);
            stat(operand,&sta);
            pwd=(char*)mmap(NULL,sta.st_size,PROT_READ,MAP_SHARED,fd,0);
            secret=(char*)MD5((unsigned char*)pwd,(unsigned long)sta.st_size,NULL);
            munmap(pwd,sta.st_size);
            close(fd);
            dataLen=strlen(secret);
            send_n(socketfd,&dataLen,sizeof(int));
            send_n(socketfd,secret,dataLen);
            recv_n(socketfd,&ret,sizeof(int));
            if(-1==ret){//存在相同名的文件夹,不能puts
                cout<<"There is a directory with same name.Puts failed"<<endl;
                continue;
            }
            else if(1==ret){//文件存在,断点续传
                recv_n(socketfd,&fileSize,sizeof(long));
            }
            else if(0==ret){
                fileSize=0;
            }
            else if(ret==2){//虚拟目录存在相同文件
                cout<<"100.00%"<<endl;
                cout<<"puts success"<<endl;
                continue;
            }
            ret=transFile(socketfd,operand,fileSize);
            if(0==ret) cout<<"puts success"<<endl;
            else if(-1==ret) perror("puts failed");*/
        }
        else if(strcmp(operate,"gets")==0){
            dataLen=recv_n(socketfd,&ret,sizeof(int));
            if(-1==dataLen){
                cout<<"Connection close"<<endl;
                return -1;
            }
            if(-1==ret){//文件不存在,gets失败
                cout<<"No such file or directory"<<endl;
                continue;
            }

            //把命令传递给子线程
            strcpy(action,buf);
            pthread_create(&threadid,NULL,threadFunc,action);

            /*
            ret=fileExist((char*)".",operand,'-');
            send_n(socketfd,&ret,sizeof(int));
            if(1==ret){//文件存在,进行断点续传
                stat(operand,&sta);
                fileSize=sta.st_size;
                send_n(socketfd,&fileSize,sizeof(long));
            }
            else{
                fileSize=0;
            }
            ret=recvFile(socketfd,operand,fileSize);
            if(0==ret) cout<<"gets success"<<endl;
            else if(-1==ret) perror("gets failed");
            */
        }
        else if(strcmp(operate,"remove")==0){
            dataLen=recv_n(socketfd,&ret,sizeof(int));
            if(-1==dataLen){
                cout<<"Connection close"<<endl;
                return -1;
            }
            if(-1==ret){
                cout<<"file or directory not exists"<<endl;
            }
            continue;
        }
        else if(strcmp(operate,"pwd")==0){
            memset(buf,0,sizeof(buf));
            recv_n(socketfd,&dataLen,sizeof(int));
            ret=recv_n(socketfd,buf,dataLen);
            if(-1==ret){
                cout<<"Connection close"<<endl;
                return -1;
            }
            cout<<buf<<endl;
        }
        else if(strcmp(operate,"mkdir")==0){
            dataLen=recv_n(socketfd,&ret,sizeof(int));
            if(-1==dataLen){
                cout<<"Connection close"<<endl;
                return -1;
            }
            if(-1==ret){
                cout<<"Directory or file exist"<<endl;
            }
        }
    }
    close(socketfd);
    return 0;
}

//puts和gets命令
void* threadFunc(void* p){
    //建立数据连接
    char buf[128]={0};
    strcpy(buf,(char*)p);
    char operate[32]={0};
    char operand[96]={0};
    char *pwd,*secret;
    int dataLen,fd;
    long fileSize=0;
    struct stat sta;
    int socketfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in sock;
    memset(&sock,0,sizeof(sockaddr_in));
    sock.sin_family=AF_INET;
    sock.sin_port=htons(atoi(port));
    sock.sin_addr.s_addr=inet_addr(ip);
    int ret=connect(socketfd,(struct sockaddr*)&sock,sizeof(sockaddr_in));
    if(-1==ret){
        perror("connect");
        return (void*)-1;
    }

    //传输命令,先发送2
    ret=2;
    send_n(socketfd,&ret,sizeof(int));

    dataLen=strlen(token);
    ret=send_n(socketfd,&dataLen,sizeof(int));
    if(-1==ret){
        cout<<"Connection close"<<endl;
        close(socketfd);
        return (void*)-1;
    }
    ret=send_n(socketfd,token,dataLen);
    if(-1==ret){
        cout<<"Connection close"<<endl;
        close(socketfd);
        return (void*)-1;
    }
    recv_n(socketfd,&ret,sizeof(int));
    if(-1==ret){//认证失败
        cout<<"puts failed"<<endl;
        close(socketfd);
        return (void*)-1;
    }
    //服务端队满
    recv_n(socketfd,&ret,sizeof(int));
    if(-1==ret){
        cout<<"puts failed"<<endl;
        close(socketfd);
        return (void*)-1;
    }
    //重传命令
    dataLen=strlen(buf);
    send_n(socketfd,&dataLen,sizeof(int));
    send_n(socketfd,buf,dataLen);

    splitChar(buf,operate,operand);

    if(strcmp(operate,"puts")==0){
        //生成文件Hash值
        fd=open(operand,O_RDONLY);
        stat(operand,&sta);
        pwd=(char*)mmap(NULL,sta.st_size,PROT_READ,MAP_SHARED,fd,0);
        secret=(char*)MD5((unsigned char*)pwd,(unsigned long)sta.st_size,NULL);
        munmap(pwd,sta.st_size);
        close(fd);
        dataLen=strlen(secret);
        send_n(socketfd,&dataLen,sizeof(int));
        send_n(socketfd,secret,dataLen);
        recv_n(socketfd,&ret,sizeof(int));
        if(2==ret){//秒传
            cout<<"100.00%"<<endl;
            cout<<"puts success"<<endl;
            goto end;
        }
        else if(1==ret){//断点续传
            recv_n(socketfd,&fileSize,sizeof(long));
        }
        else if(0==ret){
            fileSize=0;
        }
        ret=transFile(socketfd,operand,fileSize);
        if(0==ret) cout<<"puts success"<<endl;
        else if(-1==ret) perror("puts failed");

    }
    else if(strcmp(operate,"gets")==0){
        ret=fileExist((char*)".",operand,'-');
        send_n(socketfd,&ret,sizeof(int));
        if(1==ret){//文件存在,进行断点续传
            stat(operand,&sta);
            fileSize=sta.st_size;
            send_n(socketfd,&fileSize,sizeof(long));
        }
        else{
            fileSize=0;
        }
        ret=recvFile(socketfd,operand,fileSize);
        if(0==ret) cout<<"gets success"<<endl;
        else if(-1==ret) perror("gets failed");
    }

end:
    close(socketfd);
    return (void*)0;
}

int fileExist(char* curPath,char* fileName,char fileType){
    DIR *dir=opendir(curPath);
    struct dirent *dnt;
    char buf[128]={0};
    strcpy(buf,curPath);
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

void splitChar(char* from,char* operate,char* operand){
    int fromLen=strlen(from);
    int i=0,j=0;
    while(from[i]==' '&&i<fromLen) i++;
    while(from[i]!=' '&&i<fromLen) operate[j++]=from[i++];
    while(from[i]==' '&&i<fromLen) i++;
    j=0;
    while(operate[i]!=' '&&i<fromLen) operand[j++]=from[i++];
}

