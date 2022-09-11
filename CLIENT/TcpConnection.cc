#include"TcpConnection.hpp"
#include<string.h>
#include<sstream>
#include<iostream>
using std::cout;
using std::ostringstream;

TcpConnection::TcpConnection(int fd)
:_sock(fd)
,_sockIO(fd)
,_localAddr(getLocalAddr())
,_peerAddr(getPeerAddr())
,_shutdownWrite(false)
{}

TcpConnection::~TcpConnection(){

}

//读取一行
string TcpConnection::receive(){
    char buff[65536]={0};
    _sockIO.readline(buff,sizeof(buff));
    return string(buff);
}

//读取n个字节
// void TcpConnection::recvn(char *buff,int datalen){
//     _sockIO.readn(buff,datalen);
// }

//接收火车头
int TcpConnection::recvDatalen(){
    char buff[4]={0};
    int len=0;
    _sockIO.readn(buff,4);
    memcpy(&len,buff,4);
    return len;
}

string TcpConnection::recvMsg(){
    int len=recvDatalen();
    char buff[128]={0};
    _sockIO.readn(buff,len);
    return string(buff);
}



void TcpConnection::send(const string &msg){
    _sockIO.writen(msg.c_str(),msg.size());
}

void TcpConnection::sendTrain(Train_t &train){
    int ret=::send(_sock.fd(),&train,4+train.len,0);
    //cout<<"TcpConnection::sendTrain："<<"发送"<<ret<<"个字节"<<endl;
    if(-1==ret){
        perror("sendTrain");
        return;
    }
}

string TcpConnection::toString() const{
    ostringstream oss;
    oss<<"TCP connection: "
        <<_peerAddr.ip()<<": "<<_peerAddr.port()<<"--->"
        <<_localAddr.ip()<<": "<<_localAddr.port();
        
    return oss.str();
}

InetAddress TcpConnection::getLocalAddr(){
    struct sockaddr_in localaddr;
    socklen_t len=sizeof(localaddr);
    memset(&localaddr,0,sizeof(localaddr));
    //根据socket连接的fd获取本地的网络地址信息
    int ret=getsockname(_sock.fd(),(struct sockaddr*)&localaddr,&len);
    if(ret==-1){
        perror("getsockname");
    }
    return InetAddress(localaddr);
}

InetAddress TcpConnection::getPeerAddr(){
    struct sockaddr_in peeraddr;
    socklen_t len=sizeof(peeraddr);
    memset(&peeraddr,0,sizeof(peeraddr));
    //根据socket连接的fd获取对端的网络地址信息
    int ret=getpeername(_sock.fd(),(struct sockaddr*)&peeraddr,&len);
    if(ret==-1){
        perror("getsockname");
    }
    return InetAddress(peeraddr);
}

void TcpConnection::handleConnectionCanllBack(){
    if(_onConnection){
        _onConnection(shared_from_this());
    } 
}

void TcpConnection::handleMessageCallBack(){
    if(_onMessage){
        _onMessage(shared_from_this());
    }
}

void TcpConnection::handleCloseCallBack(){
    if(_onClose){
        _onClose(shared_from_this());
    }
}

bool TcpConnection::isClosed() const{
    char buff[128]={0};
    int ret=-1;
    do{
        ret=recv(_sock.fd(),buff,sizeof(buff),MSG_PEEK);
    }while(-1==ret && errno==EINTR);
    
    return 0==ret; //如果ret==0,说明连接已经段开
}