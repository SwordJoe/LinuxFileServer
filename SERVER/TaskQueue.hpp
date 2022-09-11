#pragma once
#include"MutexLock.hpp"
#include"Condition.hpp"
#include"Task.hpp"
#include<queue>
using namespace std;

using ElemType=Task;

class TaskQueue
{
public:
    TaskQueue(size_t capacity);

    bool empty() const;
    bool full() const;
    void push(ElemType &&elem);
    ElemType pop();
    void wakeupAll();
    
private:
    queue<ElemType> _que;
    size_t _capacity;
    MutexLock _mutex;
    Condition _notFull;
    Condition _notEmpty;
    bool _wakeupFlag;
};