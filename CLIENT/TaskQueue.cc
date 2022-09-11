#include"TaskQueue.hpp"

TaskQueue::TaskQueue(size_t capacity)
:_capacity(capacity)
,_mutex()
,_notFull(_mutex)
,_notEmpty(_mutex)
,_wakeupFlag(false)
{}

bool TaskQueue::empty() const{
    return _que.size()==0;
}

bool TaskQueue::full() const{
    return _que.size()==_capacity;
}

void TaskQueue::push(ElemType &&elem){
    {//利用语句块控制加锁的粒度，语句块结束对象就会被销毁
        MutexLockGuard autolock(_mutex);       //自动加锁解锁的对象

        while(full()){
            _notFull.wait();        //当条件变量满足被唤醒时，会先加锁
        }
        _que.push(std::move(elem));
    }
    _notEmpty.notify();
}

ElemType TaskQueue::pop(){
    ElemType tmp=nullptr;
    
    MutexLockGuard autolock(_mutex);

    while(_wakeupFlag==false && empty()){   //当唤醒标志为假且任务队列为空时，才阻塞等待_notEmpty条件变量
        _notEmpty.wait();
    }
    if(_wakeupFlag==false){     //被唤醒有两种可能，一种时条件变量满足，另一种是被强制唤醒，所以需要判断一下是哪一种，被强制唤醒的不需要取任务
        tmp=_que.front();
        _que.pop();
        _notFull.notify();
        return tmp;
    }
    else{
        return nullptr;
    } 
}

void TaskQueue::wakeupAll(){
    _wakeupFlag=true;
    _notEmpty.notifyall();
}