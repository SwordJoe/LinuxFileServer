#pragma once
#include<iostream>
#include<string>
#include<cstring>
#include<hiredis/hiredis.h>
using namespace std;


class Redis
{
public:
    Redis(){connect("127.0.0.1",6379);}
    ~Redis(){
        _connect=nullptr;
        _reply=nullptr;
    }
    bool connect(string host,int port){
        _connect=redisConnect(host.c_str(),port);
        if(_connect!=nullptr && _connect->err){
            printf("connect redis error:%s\n",_connect->errstr);
            return false;
        }
        return true;
    }

    string get(string key){
        _reply=(redisReply*)redisCommand(_connect,"GET %s",key.c_str());
        if(_reply->str==nullptr){
            return string("NULL");
        }
        string ret=_reply->str;
        freeReplyObject(_reply);
        return ret;
    }

    void set(string key,string value){
        redisCommand(_connect,"SET %s %s",key.c_str(),value.c_str());
    }

    void del(string key){
        redisCommand(_connect,"DEL %s",key.c_str());
    }
private:
    redisContext* _connect;
    redisReply* _reply;
};