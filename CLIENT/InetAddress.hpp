#pragma once
#include<netinet/in.h>
#include<string>
using std::string;

class InetAddress
{
public:
    InetAddress(unsigned int port,const string &ip);
    InetAddress(const struct sockaddr_in &addr);

    string ip() const;
    unsigned short port() const;
    struct sockaddr_in *getInetAddressPtr(){return &_addr;}

private:
    struct sockaddr_in _addr;
};