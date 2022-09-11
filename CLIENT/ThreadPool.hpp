#pragma once
#include"Thread.hpp"
#include"TaskQueue.hpp"
#include"Task.hpp"
#include<vector>
#include<memory>
using std::vector;
using std::unique_ptr;

class ThreadPool
{
    friend class WorkerThread;
public: 
    ThreadPool(size_t threadNum,size_t capacity);
    ~ThreadPool();
    void start();
    void stop();
    void addTask(Task &&task);

private:
    void workerDoTask();
    Task getTask();

private:
    vector<unique_ptr<Thread>> _threads;    
    size_t _threadNum;
    size_t _capacity;   //任务队列的数量
    TaskQueue _taskQue;
    bool _isExit; //线程池退出的标志位
};