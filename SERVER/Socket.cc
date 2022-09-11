#include"Socket.hpp"
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/types.h>

Socket::Socket(){
    _fd=socket(AF_INET,SOCK_STREAM,0);
    if(_fd<0){
        perror("socket");
    }
}   

Socket::Socket(int fd)
:_fd(fd)
{}

Socket::~Socket(){
    close(_fd);
}

void Socket::shutdownWrite(){
    shutdown(_fd,SHUT_WR);
}   

void Socket::setNonblock(){
    int flag=fcntl(_fd,F_GETFL,0);
    flag|=O_NONBLOCK;
    fcntl(_fd,F_SETFL,flag);
}