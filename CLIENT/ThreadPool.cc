#include"ThreadPool.hpp"
#include"Task.hpp"
#include<unistd.h>
#include<iostream>
using namespace std;

ThreadPool::ThreadPool(size_t threadNum,size_t capacity)
:_threadNum(threadNum)
,_capacity(capacity)
,_taskQue(capacity)
,_isExit(false)
{
    _threads.reserve(_threadNum);
}

ThreadPool::~ThreadPool(){
    if(_isExit){
        stop();
    }
}

void ThreadPool::start(){
    for(size_t idx=0;idx<_threadNum;++idx){
        unique_ptr<Thread> up(new Thread(std::bind(&ThreadPool::workerDoTask,this)));
        _threads.push_back(std::move(up));
    }

    for(auto &pthread:_threads){
       pthread->start();
    }
}

void ThreadPool::stop(){
    if(!_isExit){    
        //1. 先判断任务队列中是否还有任务，如果有，需要等一等
        while(!_taskQue.empty()){
            sleep(1);
        }
        //2. 如果任务队列中的的任务都被取走了，则可以考虑退出线程池
        _isExit=true;
        
        //3. 退出线程池之前，要先唤醒所有线程，防止有些线程阻塞在getTask()上(实际阻塞在更底层的TaskQueue::pop()上)
        _taskQue.wakeupAll();

        //4. 保证所有子线程都被唤醒后，join()回收每个子线程
        _isExit=true;   //将线程池退出标志位置true
        for(auto &pthread:_threads){    //回收每个子线程
            pthread->join();
            cout<<"回收成功一个"<<endl;
        }   
    }
     
}

void ThreadPool::addTask(Task &&task){
    if(task!=nullptr){
        _taskQue.push(std::move(task));
    }
}

Task ThreadPool::getTask(){
    return _taskQue.pop();
}

void ThreadPool::workerDoTask(){
    //从任务队列不断取出任务，但实际上调用此函数的是WorkerThread对象
    while(_isExit==false){
        Task task=getTask();
        if(task){
            task();
        }
    }
}