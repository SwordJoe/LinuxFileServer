#include"Thread.hpp"
#include<errno.h>
#include<stdio.h>
#include<iostream>
using std::cout;
using std::endl;


void Thread::start(){
    if(pthread_create(&_pthid,nullptr,threadFunc,this)!=0){
        perror("pthread_create");
        return;
    }
    _isRunning=true;
}
 
void* Thread::threadFunc(void *arg){
    //线程入口函数唯一要做的事就是调用回调函数
    Thread* pthread=static_cast<Thread*>(arg);
    if(pthread){
        //cout<<"child thread:"<<pthread_self()<<endl;
        pthread->_cb();
    }
    return nullptr;
}

void Thread::join(){
    if(_isRunning){
        pthread_join(_pthid,nullptr);
        _isRunning=false;
    }
}

Thread::~Thread(){
    if(_isRunning){
        pthread_detach(_pthid);
        _isRunning=false;
    }
}


