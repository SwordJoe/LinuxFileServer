#include"EventLoop.hpp"
#include"Acceptor.hpp"
#include"TcpConnection.hpp"
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<stdlib.h>
#include<iostream>
using namespace std;


EventLoop::EventLoop(Acceptor &acceptor)
:_epfd(createEpollFd())
,_eventfd(createEventfd())
,_acceptor(acceptor)
,_isLooping(false)
,_evtList(1024)
{
    addEpollReadFd(_acceptor.lfd(),false);    //将负责监听的文件描述符lfd加入epoll红黑树
    addEpollReadFd(_eventfd,false);
}

EventLoop::~EventLoop(){
    close(_epfd);
}

void EventLoop::loop(){
    _isLooping=true;
    while(_isLooping){
        waitEpollFd();
    }
}

//loop和unloop要运行在不同的线程
void EventLoop::unloop(){
    _isLooping=false;
}

void EventLoop::registerInLoop(SendFunctor &&cb){
    //EventLoop先将回调函数注册到自己的pendingFunctor上，但其实这个runInLoop函数是由TcpConnection对象调用的
    {
        MutexLockGuard autoLock(_mutex);
        _sendFunctors.push_back(std::move(cb));
    }
    //再通知EventLoop自己，即Reactor所在的IO线程，Reactor线程会执行上面刚注册过来的TcpConnetcion的发送函数
    wakeup();
}

int EventLoop::createEpollFd(){
    int epfd=epoll_create1(0);
    if(epfd==-1){
        perror("epfd");
    }
    return epfd;
}

int EventLoop::createEventfd(){
    int fd=eventfd(0,0);
    if(fd<0){
        perror("eventfd");
    }
    return fd;
}

//将文件描述符添加到epoll监听队列
void EventLoop::addEpollReadFd(int fd,bool flag){
    struct epoll_event ev;
    ev.data.fd=fd;
    if(flag){
        ev.events=EPOLLIN|EPOLLIN|EPOLLONESHOT;
    }
    else{
        ev.events=EPOLLIN|EPOLLET;
    }

    int ret=epoll_ctl(_epfd,EPOLL_CTL_ADD,fd,&ev);
    if(ret==-1){
        perror("epoll_ctl");
    }
}

//将文件描述符从epoll监听队列移除
void EventLoop::delEpollReadFd(int fd){
    struct epoll_event ev;
    ev.data.fd=fd;
    //ev.events=EPOLLIN;
    int ret=epoll_ctl(_epfd,EPOLL_CTL_DEL,fd,&ev);
    if(ret==-1){
        perror("epoll_ctl");
    }
}

//epoll循环监听
void EventLoop::waitEpollFd(){
    int nready=0;

    do{
        nready=epoll_wait(_epfd,&_evtList[0],_evtList.size(),5000);
    }while(nready==-1 && errno==EINTR);

    if(nready==-1){ //说明此时出错不是中断错误，而是epoll_wait本身出错
        perror("epoll_wait");
        exit(0);
    }
    else if(nready==0){     //超时
        //cout<<"epoll_wait overtime"<<endl;
    }
    else{       //返回就绪的文件描述符个数
        //若nready==_evtList.size()时，需要考虑扩容的操作
        if(nready==_evtList.size()){
            _evtList.resize(2*nready);
        }
        for(int idx=0;idx<nready;++idx){
            int fd=_evtList[idx].data.fd;
            if(fd==_acceptor.lfd()){    //监听客户端的fd有事件发生，说明有新连接
                if(_evtList[idx].events & EPOLLIN){
                    cout<<"新连接事件触发,";
                    handleNewConnection();
                }
            }
            else if(fd==_eventfd){  //_eventfd上有事件
                if(_evtList[idx].events & EPOLLIN){
                    handleRead();           //先对计算线程的通知进行处理,读取一下
                    doSendFunctors();    //再去执行回调函数
                }
            }
            else{       //客户端fd有事件发生，说明有消息从客户端发送过来
                if(_evtList[idx].events &EPOLLIN){
                    cout<<"客户端"<<fd<<"消息事件触发一次"<<endl;
                    handleMessage(fd);
                }
            }
        }
    }
}

//处理新连接
void EventLoop::handleNewConnection(){
    int peerfd=_acceptor.accept();
    cout<<"新客户端为："<<peerfd<<endl;
    //将新连接fd加入到epoll监听队列
    addEpollReadFd(peerfd,true);     
    //根据获取到的peerfd创建一个TcpConnectionPtr对象
    TcpConnectionPtr conn(new TcpConnection(peerfd,this));
    //给新连接TcpConnectionPtr设置回调三个事件的回调函数
    conn->setAllCallBacks(_onConnection,_onMessage,_onClose);      
    //将TcpConnetionPtr对象加入TCP连接池_conns
    _conns.insert(make_pair(peerfd,conn));
    //新的TCP连接执行连接回调函数
    conn->handleConnectionCanllBack();  //执行新连接到来时的回调函数
}

//处理消息
void EventLoop::handleMessage(int fd){
    //先根据fd查找到TcpConnection对象
    auto iter=_conns.find(fd);

    if(iter!=_conns.end()){
        bool isClosed=iter->second->isClosed();     //判断一下该连接是否断开
        if(isClosed){   //如果连接断开，就调用连接关闭回调函数
            iter->second->handleCloseCallBack();    
            delEpollReadFd(fd);     //从epoll监听队列中删除该连接的fd
            _conns.erase(iter);     //从TCP连接池中删除该连接
        }
        else{   //如果没有断开，就调用处理消息回调函数
            iter->second->handleMessageCallBack();
        }
    }
}

void EventLoop::handleRead(){
    uint64_t num=0;
    int ret=read(_eventfd,&num,sizeof(num));
    if(ret!=sizeof(num)){
        perror("read");
    }
} 

void EventLoop::wakeup(){
    uint64_t one=1;
    int ret=write(_eventfd,&one,sizeof(one));
    if(ret!=sizeof(one)){
        perror("write");
    }
}

void EventLoop::doSendFunctors(){
    vector<SendFunctor> tmp;
    {
        MutexLockGuard autoLock(_mutex);
        tmp.swap(_sendFunctors);     //vector中swap的时间复杂度是O(1)
    }
    for(auto &functor:tmp){
        functor();
        //cout<<"执行一次"<<endl;
    }
}

void EventLoop::setEpollOneShot(int fd){
    delEpollReadFd(fd);     //先将fd从监听树上摘除
    struct epoll_event ev;
    ev.data.fd=fd;
    ev.events=EPOLLIN|EPOLLET|EPOLLONESHOT;     //将fd设置为ET+ONESHOT模式再添加到监听树上
    int ret=epoll_ctl(_epfd,EPOLL_CTL_ADD,fd,&ev);
    if(-1==ret){
        perror("epoll_ctl");
    }
    else{
        cout<<"设置ONESHOT成功"<<endl;
    }
}

void EventLoop::resetEpoll(int fd){     
    struct epoll_event ev;
    ev.data.fd=fd;
    ev.events=EPOLLIN|EPOLLET|EPOLLONESHOT;   
    int ret=epoll_ctl(_epfd,EPOLL_CTL_MOD,fd,&ev);
    if(-1==ret){
        perror("epoll_ctl,MOD");
    }
    else{
        cout<<"RESET一次"<<endl;
    }
}