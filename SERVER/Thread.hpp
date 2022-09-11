#pragma once
#include"NonCopyable.hpp"
#include<pthread.h>
#include<functional>
using std::function;

using ThreadCallBack=function<void()>;
//Thread本身是一个具体类，不再是抽象类
class Thread:NonCopyable
{
public:
	Thread(ThreadCallBack &&cb) //参数用右值引用的形式
	: _pthid(0)
	, _isRunning(false)
	, _cb(std::move(cb))
	{}
	~Thread();

	void start();
	void join();

private:
	static void * threadFunc(void *);

private:
	pthread_t _pthid;
	bool _isRunning;
	ThreadCallBack _cb;
};
