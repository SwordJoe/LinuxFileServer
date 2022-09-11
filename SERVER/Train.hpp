#pragma once
#include<string>
#include<string.h>
using std::string;

struct Train_t{
  Train_t(){}
  Train_t(int datalen,string Msg):len(datalen){memcpy(buf,Msg.c_str(),Msg.size());}
  int len=0;
  char buf[65536]={0};
};