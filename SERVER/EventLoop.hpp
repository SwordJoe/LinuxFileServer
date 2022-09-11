#pragma once
#include"MutexLock.hpp"
#include<vector>
#include<map>
#include<memory>
#include<functional>
using namespace std;

class TcpConnection;
class Acceptor;
using TcpConnectionPtr=shared_ptr<TcpConnection>;
using TcpConnectionCallBack=function<void(const TcpConnectionPtr&)>;
using SendFunctor=function<void()>;     //发送信息的回调函数

class EventLoop
{
public:
    EventLoop(Acceptor &acceptor);
    ~EventLoop(); 
    void loop();
    void unloop();
    void registerInLoop(SendFunctor &&);    //星空代码里写的是runInLooop
    void setAllCallBacks(TcpConnectionCallBack &&cb1,
                        TcpConnectionCallBack &&cb2,
                        TcpConnectionCallBack &&cb3)
    {
        _onConnection=std::move(cb1);
        _onMessage=std::move(cb2);
        _onClose=std::move(cb3);
    }
    void setEpollOneShot(int fd);   //22/1/8添加
    void resetEpoll(int fd);

private:
    int createEpollFd();
    int createEventfd();
    void addEpollReadFd(int fd,bool flag); //将文件描述符添加到监听队列
    void delEpollReadFd(int fd);    
    void waitEpollFd();
    void handleNewConnection();
    void handleMessage(int fd);
    void handleRead();          //处理eventfd的通知
    void wakeup();              //通知eventfd
    void doSendFunctors();   //执行发送操作的回调函数
    

private:
    int _epfd;
    int _eventfd;
    Acceptor &_acceptor;
    bool _isLooping;
    vector<struct epoll_event> _evtList;
    map<int,TcpConnectionPtr> _conns;

    TcpConnectionCallBack _onConnection;
    TcpConnectionCallBack _onMessage;
    TcpConnectionCallBack _onClose;

    MutexLock _mutex;
    vector<SendFunctor> _sendFunctors;
};