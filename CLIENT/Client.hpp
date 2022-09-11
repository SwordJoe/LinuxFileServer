#include"Head.h"
#include"Connector.hpp"
#include"TcpConnection.hpp"
#include"../ProtoMsg/Message.pb.h"
#include<sys/mman.h>
#include<sys/sendfile.h>
#include<sys/types.h>
#include<dirent.h>
#include<string>
//#include<deque>
#include<iostream>
#include<sstream>
#include<set>
using namespace std;

class Client
{
public:
    Client(const string &downloadPath):_downloadPath(downloadPath){}
    void connect();
    void handleCmd();
    void Register_Login();
    void ls();
    void cd(string &arg);
    void mkdir(string &arg);
    void upload();
    void put(string &arg);
    void get(string &arg);
    int handleCmd(const string &input,string &cmd,string &args);
    string name(){return _userName;}
    string path(){return _curDirPath;}
    
private:
    void sendFile(int fileFd,int peerfd,int fileSize,int breakPoint);
    void recvFile(string &filePathName,int fileSize,int peerfd);
private:
    Socket _sock;
    Connector _connector;
    TcpConnectionPtr _mainTcpConn;
    string _userName;
    string _curDirPath;     //该目录是绝对路径
    string _downloadPath;
};

void Client::connect(){
    int ret=_connector.Connect(_sock.fd());
    if(-1==ret){
        perror("connect");
        exit(0);
    }
    else if(0==ret){
        cout<<"建立连接"<<endl;
    }
    _mainTcpConn=make_shared<TcpConnection>(_sock.fd());
}

//登录&注册
void Client::Register_Login(){
    cout<<loginPage;
    while(1){
        string str;
        getline(cin,str);
        if("1"==str){       //登录
            system("reset");
            //cout<<loginPage;
            string userName,passwd;
            cout<<"用户名:";    getline(cin,userName);
            cout<<"密码：";     getline(cin,passwd);

            Cmd cmd;
            cmd.set_cmdid(2);       //cmdid=2,登录
            auto pUserInfo=new UserInfo();
            pUserInfo->set_username(userName);
            pUserInfo->set_passwd(passwd);
            cmd.set_allocated_userinfo(pUserInfo);
            
            string msg=cmd.SerializeAsString();
            Train_t train(msg.size(),msg);
            _mainTcpConn->sendTrain(train);

            msg=_mainTcpConn->recvMsg();    //从服务端接收反馈消息
            if(msg=="登录成功"){
                system("reset");
                cout<<msg<<endl;
                //_curDirPath.push_back(userName);
                _userName=userName;
                //_curDirPath="/"+userName;   //登录成功就将当前路径设置为用户的"家目录"
                cout<<_userName<<":/"<<_curDirPath<<"> ";
                break;
            }
            else{
                system("reset");
                cout<<"用户名或密码错误"<<",请重新输入"<<endl;
                cout<<loginPage;
                continue;
            }
        }
        else if("2"==str){      //注册
            system("reset");
            //cout<<loginPage;
            string userName,passwd;
            cout<<"用户名:";    getline(cin,userName);
            cout<<"密码：";     getline(cin,passwd);

            Cmd cmd;
            cmd.set_cmdid(1);       //cmdid=1,注册

            auto pUserInfo=new UserInfo();
            pUserInfo->set_username(userName);
            pUserInfo->set_passwd(passwd);
            cmd.set_allocated_userinfo(pUserInfo);

            string msg=cmd.SerializeAsString();       //protobuf序列化为string字节流
            Train_t train(msg.size(),msg);      //封装成"小火车"
            _mainTcpConn->sendTrain(train);     //发送注册消息给服务器
            
            string str=_mainTcpConn->recvMsg();
            if(str=="用户已存在"){
                system("reset");
                cout<<str<<",请重新输入"<<endl;
                cout<<loginPage;
                continue;
            }
            else if(str=="注册失败"){
                system("reset");
                cout<<str<<",请重新输入"<<endl;
                break;
            }
            else if(str=="注册成功"){
                system("reset");
                cout<<str<<endl;
                cout<<loginPage;
                continue;
            }
        }
        else if("q"==str){
            exit(0);
        }
        else{
            system("reset");
            cout<<"请选择正确的选项"<<endl;
            cout<<loginPage;
            continue;
        }
    }
}

int Client::handleCmd(const string &input,string &cmd,string &arg){
    int ret=0;
    int pos=input.find_first_of(' ');
    cmd=string(input,0,pos);
    if(pos==string::npos){
        arg="null";
    }
    else{
        string tmp(input,pos);
        int idx=tmp.find_first_not_of(' ');
        if(idx==string::npos){
            arg="null";
        }
        else{
            arg=string(tmp,idx);
        }
    }

    if(arg!="null"){
        int pos=arg.find(' ');
        if(pos!=string::npos){
            string tmpStr(arg,pos);
            int idx=tmpStr.find_first_not_of(' ');
            if(idx!=string::npos){
                //cout<<"命令参数有问题"<<" arg="<<arg<<endl;
                ret=-2;
            }
        }
    }
    set<string> cmdSet{"ls","cd","pwd","get","put","mkdir","rm"};
    if(cmdSet.find(cmd)==cmdSet.end()){
        ret=-1;
        //cout<<"无该命令"<<endl;
    }
    int RET;
    switch(ret){
        case 0: RET=1; break; 
        case -1:{
            cout<<"无该命令,请重新输入"<<endl;
            RET=0;
            break;
        }
        case -2:{
            cout<<"请输入正确的命令参数"<<endl;
            RET=0;
            break;
        }
    }
    //cout<<_userName<<":/"<<_curDirPath<<"> ";
    return RET;
}

void Client::ls(){
    //cout<<"here1"<<endl;
    Cmd cmd;
    cmd.set_cmdid(3);   //设置ls的命令id为3

    auto pLSinfo=new LS();
    string path="/"+_userName+"/"+_curDirPath;
    pLSinfo->set_curdirpath(path);
    cmd.set_allocated_lsinfo(pLSinfo);      //将ls信息加入到cmd中

    string msg=cmd.SerializeAsString(); //序列化消息

    Train_t train(msg.size(),msg);
    _mainTcpConn->sendTrain(train);     //发送ls命令的消息
    //cout<<"here2--"<<endl;
    //cout<<"发送"<<msg.size()<<"个字节"<<endl;
    msg=_mainTcpConn->recvMsg();    
    cout<<msg;
}

void Client::cd(string &arg){
    string path;
    if(arg=="null"){    //表示回到家目录
        path="/"+_userName;
    }
    else if(arg[0]=='/' || _curDirPath==""){
        path="/"+_userName+"/"+arg;
    }
    else{
        path="/"+_userName+"/"+_curDirPath+"/"+arg;
    }
    Cmd cmd;
    cmd.set_cmdid(4);
    auto pCdInfo=new Cd();
    pCdInfo->set_dirpath(path);
    cmd.set_allocated_cdinfo(pCdInfo);

    string msg=cmd.SerializeAsString();
    Train_t train(msg.size(),msg);
    _mainTcpConn->sendTrain(train);

    string recvMsg=_mainTcpConn->recvMsg();
    if(recvMsg=="false"){
        cout<<"无该目录，请输入正确的目录路径名"<<endl;
        return;
    } 

    recvMsg=_mainTcpConn->recvMsg();
    if(recvMsg=="fail"){
        cout<<"无该目录，请输入正确的目录路径名"<<endl;
    }
    else{
        _curDirPath=recvMsg;
    }
}

void Client::mkdir(string &arg){
    string curDirPath=_curDirPath.empty()?("/"+_userName+"/"):("/"+_userName+"/"+_curDirPath+"/");
    
    Cmd cmd;
    cmd.set_cmdid(6);
    auto pMkdirInfo=new Mkdir();
    pMkdirInfo->set_curdirpath(curDirPath);
    pMkdirInfo->set_dirname(arg);
    cmd.set_allocated_mkdirinfo(pMkdirInfo);

    string msg=cmd.SerializeAsString();
    Train_t train(msg.size(),msg);

    _mainTcpConn->sendTrain(train);

    string recvmsg=_mainTcpConn->recvMsg();
    if(recvmsg=="success"){
        cout<<"创建目成功"<<endl;
    }
    else if(recvmsg=="same"){
        cout<<"创建目录失败，该目录下已有同名目录"<<endl;
    }
}

//上传文件
void Client::put(string &arg){
    cout<<"目前线程:"<<pthread_self()<<endl;
    Socket sock;
    int val=_connector.Connect(sock.fd());
    if(-1==val){
        perror("connect");
        return;
    }
    else{
        cout<<"建立长命令连接"<<endl;
    }
    TcpConnection conn(sock.fd());


    int fileFd=open(arg.c_str(),O_RDWR);     //打开文件
    if(-1==fileFd){
        perror("open arg");
        cout<<"无该文件，请输入正确的文件名"<<endl;
        return;
    }

    //得到文件的md5码
    char buff[50]={0};
    int ret=Compute_file_md5(const_cast<char*>(arg.c_str()),buff);     
    cout<<"md5="<<buff<<endl;
    if(-1==ret){
        cout<<"Compute_file_md5 error,请重试"<<endl;
        return;
    }
    //得到文件md5码
    string md5(buff); 
    //得到当前绝对路径
    string curDirPath=_curDirPath.empty()?("/"+_userName+"/"):("/"+_userName+"/"+_curDirPath+"/");      
    //得到文件大小
    struct stat statbuf;
    stat(const_cast<char*>(arg.c_str()),&statbuf);
    int fileSize=statbuf.st_size;

    Cmd cmd;
    cmd.set_cmdid(9);
    auto pPutInfo=new Put();
    pPutInfo->set_curdirpath(curDirPath);   //当前目录
    pPutInfo->set_filesize(fileSize);       //文件大小
    pPutInfo->set_md5(md5);                 //文件md5码
    pPutInfo->set_filename(arg);            //文件名
    cmd.set_allocated_putinfo(pPutInfo);

    string msg=cmd.SerializeAsString();
    Train_t train(msg.size(),msg);
    conn.sendTrain(train);          //发送put命令的相关信息给服务端

    //判断有无同名文件
    msg=conn.recvMsg();
    if(msg=="same"){
        cout<<"当前目录有同名文件"<<endl;
        cout<<_userName<<":/"<<_curDirPath<<"> ";
        return;
    }

    msg=conn.recvMsg();
    cout<<msg<<endl;
    if(msg=="秒传"){
        cout<<"极速秒传"<<endl;
        cout<<_userName<<":/"<<_curDirPath<<"> ";
        return;
    }
    else if(msg=="断点续传"){
        int breakPoint=atoi(conn.recvMsg().c_str());    //接收断点续传的偏移量
        cout<<"偏移量:"<<breakPoint<<endl;
        sendFile(fileFd,sock.fd(),fileSize,breakPoint);
    }
    else if(msg=="普通传输"){     //普通传输
        string msg=conn.recvMsg();
        int breakPoint=((msg=="null")?0:atoi(msg.c_str()));
        sendFile(fileFd,sock.fd(),fileSize,breakPoint);   
    }
    cout<<_userName<<":/"<<_curDirPath<<"> ";
}

//下载文件
void Client::get(string &arg){
    cout<<"目前线程:"<<pthread_self()<<endl;
    Socket sock;
    int val=_connector.Connect(sock.fd());
    if(-1==val){
        perror("connect");
        return;
    }
    else{
        cout<<"建立长命令连接"<<endl;
    }
    TcpConnection conn(sock.fd());

    //得到当前绝对路径
    string curDirPath=_curDirPath.empty()?("/"+_userName+"/"):("/"+_userName+"/"+_curDirPath+"/");
    
    //检查当前目录有无相同文件名
    DIR *pdir=opendir(_downloadPath.c_str());
    if(pdir==nullptr){
        perror("opendir");
        return;
    }
    struct dirent *pdirent;
    bool flag=false;
    while((pdirent=readdir(pdir))!=nullptr){
        if(pdirent->d_name==arg){
            cout<<"当前路径有重名文件"<<endl;
            return;
        }
    }
    cout<<"当前目录无重名文件"<<endl;

    //建立下载命令的相关信息
    Cmd cmd;
    cmd.set_cmdid(8);
    auto pGetInfo=new Get();
    pGetInfo->set_curdirpath(curDirPath);   //当前目录
    pGetInfo->set_filename(arg);            //文件名
    cmd.set_allocated_getinfo(pGetInfo);
    string msg=cmd.SerializeAsString();
    Train_t train(msg.size(),msg);
    conn.sendTrain(train);  //将下载命令的相关信息发送给客户端

    //接收服务端有无该文件的消息
    msg=conn.recvMsg();
    if(msg=="false"){
        cout<<"服务端当前目录无该文件，请输入正确的文件名"<<endl;
        return;
    }
    cout<<"服务端有该文件"<<endl;
    
    //接收文件大小
    msg=conn.recvMsg();
    off_t fileSize=atoi(msg.c_str());               //文件大小
    cout<<"文件大小为:"<<fileSize<<endl;
    int peerfd=conn.fd();                           //通信fd
    string filePathName=_downloadPath+"/"+arg;      //下载的文件路径名
    cout<<"filePathName="<<filePathName<<endl;
    recvFile(filePathName,fileSize,peerfd);
    cout<<"下载成功"<<endl;
}

void Client::sendFile(int fileFd,int peerfd,int fileSize,int offset){
    //1. 普通传输，循环读取文件数据并发送给服务端
    // Train_t train;
    // int ret;
    // lseek(fileFd,offset,SEEK_SET);
    // while(1){
    //     memset(&train,0,sizeof(train));
    //     train.len=read(fileFd,train.buf,sizeof(train.buf));
    //     ret=send(peerfd,&train,4+train.len,0);

    //     if(-1==ret){        //对端断开
    //         break;
    //     }
    //     if(0==train.len){   //读取文件完毕
    //         break;
    //     }
    // }

    //零拷贝mmap，文件映射
    // char *pMap=(char*)mmap(NULL,fileSize,PROT_READ|PROT_WRITE,MAP_SHARED,fileFd,0);
    // int ret=send(peerfd,pMap+offset,fileSize-offset,0);
    // if(-1==ret){
    //     perror("send");
    //     return;
    // }

    //零拷贝sendfile，只能用于发送方
    // off_t offSet=offset;
    // ret=sendfile(peerfd,fileFd,&offSet,fileSize);
    // if(-1==ret){
    //     perror("sendfile");
    //     return;
    // }

    //零拷贝方式三: splice,需要利用管道
    int pipefd[2]={0};
    pipe(pipefd);
    off_t offSet=offset;
    while(1){
        int ret=splice(fileFd,&offSet,pipefd[1],0,4096,0);
        splice(pipefd[0],0,peerfd,0,ret,0);
        if(ret==0){
            break;
        }
    }

    cout<<"发送完毕"<<endl;
}

void Client::recvFile(string &filePathName,int fileSize,int peerfd)
{   
    int fileFd=open(filePathName.c_str(),O_CREAT|O_RDWR,0664);
    if(-1==fileFd){
        perror("open filePathName");
        return;
    }
   
    //1.普通传输，循环接收文件数据
    // lseek(fileFd,offset,SEEK_SET);
    // cout<<"偏移到:"<<offset<<endl;
    // char buff[70000]={0};
    // int datalen=0;
    // while(1){
    //     memset(buff,0,sizeof(buff));
    //     int ret=recv(cfd,&datalen,4,0);
    //     if(0==datalen || ret==0){     //接收完毕或者对端关闭连接
    //         break;
    //     }

    //     ret=recv(cfd,buff,datalen,MSG_WAITALL);
    //     //cout<<"接收"<<ret<<"个字节"<<endl;
    //     if(ret==-1){
    //         perror("recv");
    //         return;
    //     }
    //     write(fileFd,buff,ret);
    //     realSize+=ret;
    // }

    //零拷贝mmap，文件映射
    //ftruncate(fileFd,fileSize);     //给文件提前开辟空间
    // char *pMap=(char*)mmap(NULL,fileSize,PROT_READ|PROT_WRITE,MAP_SHARED,fileFd,0);
    // if(pMap==(void*)(-1)){
    //     perror("mmap");
    // }
    // ret=recv(cfd,pMap+offset,fileSize-offset,MSG_WAITALL);
    // realSize+=ret;    //接收后的文件大小
    // munmap(pMap,fileSize);

    //零拷贝splice，需要利用管道
    int pipefd[2]={0};
    pipe(pipefd);
    while(1){
        int ret=splice(peerfd,0,pipefd[1],0,4096,0);
        splice(pipefd[0],0,fileFd,0,ret,0);
        if(ret==0){
            break;
        }
    }
    close(fileFd);      //关闭文件描述符
    cout<<"接收完毕"<<endl;
}
