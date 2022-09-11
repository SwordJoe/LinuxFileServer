#pragma once

class SocketIO
{
public:
    SocketIO(int fd):_fd(fd){}

    int readn(char *buff,int len);
    int readline(char *buff,int maxlen);    //读取一行，maxlen表示最长读取的字节数
    int writen(const char *buff,int len);

private:
    int _fd;
};