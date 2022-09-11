#pragma once
#include"NonCopyable.hpp"

class Socket:NonCopyable
{
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();

    int fd() const{return _fd;} 
    void shutdownWrite();   //关闭读端
    void setNonblock();     //设置文件描述符非阻塞

private:
    int _fd;
};