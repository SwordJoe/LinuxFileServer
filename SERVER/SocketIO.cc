#include"SocketIO.hpp"
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<iostream>
using namespace std;

int SocketIO::readn(char *buff,int len){
    char *pbuf=buff;
    int left=len;
    int ret=0;
    while(left>0){
        ret=recv(_fd,pbuf,left,0);
        if(ret==0){
            break;
        }
        else if(ret==-1 && errno==EINTR){
            continue;
        }
        else if(ret==-1){
            perror("recv");
            break;
        }
        else{
            pbuf+=ret;
            left-=ret;
        }
    }
    return len-left;
}

int SocketIO::readline(char *buff,int maxlen){
    char *pbuf=buff;
    int left=maxlen-1;  //表示剩余要读取的字节数
    int total=0;
    while(left>0){
        int ret=recv(_fd,pbuf,left,MSG_PEEK); //MSG_PEEK窥探内核缓冲区数据，但是并不从中取走数据
        if(ret==0){
            break;
        }    
        else if(ret==-1 && errno==EINTR){
            continue;
        }
        else if(ret==-1){
            break;
        }
        else{   //ret>0,争取读取到一定数量字节的数据
            if(ret+total>maxlen-1){
                ret=maxlen-1-total;
            }
            for(int idx=0;idx<ret;++idx){
                if(pbuf[idx]=='\n'){
                    int sz=idx+1;
                    readn(pbuf,sz);   //读取sz个字节，即刚好读取到'\n'
                    //pbuf[sz]='\0';   //将'\n'后面的一个字节设置为'\0'
                    pbuf[sz-1]='\0';  //将'\n'设置为'\0'
                    return sz+total;
                }
            }
            //没有找到'\n'
            readn(pbuf,ret);
            pbuf+=ret;
            left-=ret;
            total+=ret;
        }
    }
    //退出while循环，表示读取maxlen长度的字节后还是没有读取到'\n'
    buff[maxlen-1]='\0';
    return maxlen-1;
}

int SocketIO::writen(const char *buff,int len){
    // cout<<"准备发送给客户端的数据是："<<buff<<endl;
    // cout<<"数据长度是："<<len<<endl;
    const char *pbuf=buff;
    int left=len;
    int ret=0;
    while(left>0){
        ret=send(_fd,pbuf,left,0);
        if(ret==-1){
            perror("send");
            break;
        }
        else{
            pbuf+=ret;
            left-=ret;
        }
    }
    return len-left;
}