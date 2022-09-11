#pragma once
#include"NonCopyable.hpp"
#include<pthread.h>

class MutexLock;
class Condition:NonCopyable
{
public:
    Condition(MutexLock &mutex);
    ~Condition();

    void wait();
    void notify();
    void notifyall();

private:
    pthread_cond_t _cond;
    MutexLock &_mutex;
};