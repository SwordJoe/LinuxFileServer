#pragma once
#include"md5.h"
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<string>
using std::string;

const string ServIP="139.196.153.114";
const unsigned int ServPort=8888;
const string loginPage("--------登录请输1-------\n--------注册请输2-------\n--------退出请输q--------\n");

struct Train_t{
  Train_t(){}
  Train_t(int datalen,string Msg):len(datalen){memcpy(buf,Msg.c_str(),Msg.size());}
  int len=0;
  char buf[65536]={0};
};

