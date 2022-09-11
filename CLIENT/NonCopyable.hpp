#pragma once

//禁止复制类
class NonCopyable       
{
protected:  //定义了protected型构造函数的类也称为抽象类
    NonCopyable(){}
    ~NonCopyable(){}

    NonCopyable(const NonCopyable&)=delete;
    NonCopyable& operator=(const NonCopyable&)=delete;
};