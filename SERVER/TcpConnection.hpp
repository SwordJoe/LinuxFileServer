#pragma once
#include"Socket.hpp"
#include"InetAddress.hpp"
#include"SocketIO.hpp"
#include"NonCopyable.hpp"
#include"Train.hpp"
#include"EventLoop.hpp"
#include<functional>
#include<memory>
using namespace std;

class TcpConnection;
using TcpConnectionPtr=shared_ptr<TcpConnection>;
using TcpConnectionCallBack=function<void(const TcpConnectionPtr&)>;

class TcpConnection
:NonCopyable
//在本类内部获取本类对象的智能指针,需要继承该辅助类
,public std::enable_shared_from_this<TcpConnection> 
{
public:
    TcpConnection(int fd,EventLoop* eventloop);
    ~TcpConnection();
 

    string receive();               //接收一行，以"\n"为结束标志
    
    inline int recvDatalen();              //接收火车头
    string recvMsg();    //接收火车车厢
    
    void send(const string &msg);
    void sendTrain(Train_t &train);
    void sendInLoop(const string &msg);
    string toString() const;
    void setAllCallBacks(const TcpConnectionCallBack &cb1,
                        const TcpConnectionCallBack &cb2,
                        const TcpConnectionCallBack &cb3)
    {
        _onConnection=cb1;
        _onMessage=cb2;
        _onClose=cb3;
    }
    void handleConnectionCanllBack();
    void handleMessageCallBack();
    void handleCloseCallBack();
    bool isClosed() const;      //判断连接是否断开
    int fd(){return _sock.fd();}
    void setEpollOneShot(){
        _loop->setEpollOneShot(_sock.fd());
    }
    void resetEpoll(){
        _loop->resetEpoll(_sock.fd());
    }
    void setCookie(string &cookie){_cookie=cookie;}
    string cookie(){return _cookie;}
private:    
    InetAddress getLocalAddr();     //根据fd获取本端的网络地址，IP+port
    InetAddress getPeerAddr();      //根据fd获取对端的网络地址，IP+port

private:
    Socket _sock;
    SocketIO _sockIO;
    InetAddress _localAddr;
    InetAddress _peerAddr;
    bool _shutdownWrite;
    EventLoop *_loop;

    TcpConnectionCallBack _onConnection;    //新连接建立时，需要处理的事
    TcpConnectionCallBack _onMessage;       //连接上有消息到达时，需要处理的事
    TcpConnectionCallBack _onClose;         //连接关闭时，需要处理的事
    string _cookie;
};