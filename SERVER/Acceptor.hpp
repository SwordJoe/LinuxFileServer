#pragma once
#include"Socket.hpp"
#include"InetAddress.hpp"
#include"NonCopyable.hpp"

class Acceptor:NonCopyable
{
public:
    Acceptor(unsigned int port,const string &ip="172.24.40.165")
    :_listenSock()
    ,_addr(port,ip)
    {}
    void ready();
    int accept();
    int lfd() const{ return _listenSock.fd();}

private:
    void setReuseAddr();
    void setReusePort();
    void bind();
    void listen();

private:
    Socket _listenSock;
    InetAddress _addr;
};
