#pragma once
#include"Acceptor.hpp"
#include"EventLoop.hpp"
#include"TcpConnection.hpp"

class TcpServer
{
public:
    TcpServer(unsigned short port,const string &ip="172.24.40.165")
    :_acceptor(port,ip)
    ,_loop(_acceptor)
    {}

    void start(){
        _acceptor.ready();
        _loop.loop();
    }

    void stop(){        //stop()与start()要运行在不同的线程
        _loop.unloop();
    }
    void setAllCallBacks(TcpConnectionCallBack &&cb1,
                        TcpConnectionCallBack &&cb2,
                        TcpConnectionCallBack &&cb3)
    {
        _loop.setAllCallBacks(std::move(cb1),std::move(cb2),std::move(cb3));
    }

private:
    Acceptor _acceptor;
    EventLoop _loop;
};