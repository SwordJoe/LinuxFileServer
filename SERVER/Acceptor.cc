#include"Acceptor.hpp"
#include<iostream>
using std::cout;
using std::endl;

void Acceptor::ready(){
    setReuseAddr();
    setReusePort();
    bind();
    listen();
}

int Acceptor::accept(){
    int peerfd=::accept(lfd(),nullptr,nullptr);
    if(peerfd==-1){
        perror("accept");
    }
    return peerfd;
}

void Acceptor::setReuseAddr(){
    int on=1;
    int ret=setsockopt(lfd(),SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    if(ret==-1){
        perror("setsockopt");
    }
}

void Acceptor::setReusePort(){
    int on=1;
    int ret=setsockopt(lfd(),SOL_SOCKET,SO_REUSEPORT,&on,sizeof(on));
    if(ret==-1){
        perror("setsockopt");
    }
}

void Acceptor::bind(){
    int ret=::bind(lfd(),(struct sockaddr*)_addr.getInetAddressPtr(),sizeof(_addr));
    if(ret<0){
        perror("bind");
    }
}

void Acceptor::listen(){
    int ret=::listen(lfd(),128);
    if(ret==-1){
        perror("listen");
    }
    cout<<"server is listening...."<<endl;
}

