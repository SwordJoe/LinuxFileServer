#include"Socket.hpp"
#include"InetAddress.hpp"
#include"Head.h"
#include<sys/types.h>
#include<sys/types.h>
#include<iostream>
using namespace std;

class Connector
{
public:
    Connector():_servAddr(ServPort,ServIP){}
    int Connect(int fd);

private:
    InetAddress _servAddr;
};

int Connector::Connect(int fd){
    socklen_t len=sizeof(_servAddr);
    int ret=connect(fd,(struct sockaddr*)_servAddr.getInetAddressPtr(),len);
    return ret;
}
