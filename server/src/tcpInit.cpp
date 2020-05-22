#include "funcpp.h"

int tcpInit(char* ip,char* port){
    int socketfd=socket(AF_INET,SOCK_STREAM,0);
    if(-1==socketfd){
        perror("socket");
        return -1;
    }
    struct sockaddr_in sock;
    memset(&sock,0,sizeof(sock));
    sock.sin_family=AF_INET;
    sock.sin_port=htons(atoi(port));
    sock.sin_addr.s_addr=inet_addr(ip);
    int reuse=1;
    setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
    int ret=bind(socketfd,(struct sockaddr*)&sock,sizeof(struct sockaddr_in));
    if(-1==ret){
        perror("bind");
        return -1;
    }
    return socketfd;
}

