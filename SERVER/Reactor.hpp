#include"Head.h"
#include"TcpServer.hpp"
#include"ThreadPool.hpp"
#include"NetDiskTask.hpp"
#include"../ProtoMsg/Message.pb.h"
#include<iostream>
#include<functional>
using namespace std;
using namespace placeholders;
extern Crud crud;

class ReactorServer
{
public:
    ReactorServer(unsigned short port,const string &ip, size_t threadNum,size_t taskSize)
    :_threadPool(threadNum,taskSize)
    ,_server(port,ip)
    {}
    
    void start(){
        _threadPool.start();
        _server.setAllCallBacks(bind(&ReactorServer::onConnection,this,_1),    //因为onConnection的参数TcpConnection只有等到创建时才会传入，所以用到占位符_1，下同
                                bind(&ReactorServer::onMessage,this,_1),
                                bind(&ReactorServer::onClose,this,_1));
        _server.start();
    }

    void onConnection(const TcpConnectionPtr &conn){
        cout<<conn->toString()<<" has connected"<<endl; 
    }

    //onMessage运行在IO线程(Reactor所在线程)，即消息的接收和发送由Reactor线程来完成，消息的处理交给线程池中的线程来做
    void onMessage(const TcpConnectionPtr &conn){
        //receive
        string msg=conn->recvMsg();
        cout<<"收到======="<<msg.size()<<"个字节"<<endl;
        Cmd cmd;
        cmd.ParseFromString(msg);
        int cmdid=cmd.cmdid();
        cout<<"cmdid="<<cmdid<<endl;
        if(cmdid==9){
            //conn->setEpollOneShot();
        }
        
        NetDiskTask task(msg,conn,crud,"/home/Joe/UserDIR");
        switch(cmdid){
            case 1: {   //注册
                string userName=cmd.userinfo().username();
                string passwd=cmd.userinfo().passwd();
                _threadPool.addTask(bind(&NetDiskTask::Register,task,userName,passwd));
                break;
            }
            case 2:{    //登录
                string userName=cmd.userinfo().username();
                string passwd=cmd.userinfo().passwd();
                _threadPool.addTask(bind(&NetDiskTask::login,task,userName,passwd));
                break;
            }
            case 3:{    //ls
                cout<<"ls命令"<<endl;
                string curDirPath=cmd.lsinfo().curdirpath();
                _threadPool.addTask(bind(&NetDiskTask::ls,task,curDirPath));
                break;
            }
            case 4:{    //cd
                string dirPath=cmd.cdinfo().dirpath();
                _threadPool.addTask(bind(&NetDiskTask::cd,task,dirPath));
                break;
            }
            case 5:{    //pwd
                break;
            }
            case 6:{    //mkdir
                string curDirPath=cmd.mkdirinfo().curdirpath();
                string dirName=cmd.mkdirinfo().dirname();
                _threadPool.addTask(bind(&NetDiskTask::mkdir,task,curDirPath,dirName));
                break;
            }
            case 7:{    //rm
                break; 
            }   
            case 8:{    //get
                _threadPool.addTask(bind(&NetDiskTask::get,task,cmd));
                break;
            }
            case 9:{    //put
                _threadPool.addTask(bind(&NetDiskTask::put,task,cmd));
                break;
            }
            case 10:{
                cout<<"跳过"<<endl;
            }
        }
    }

    void onClose(const TcpConnectionPtr &conn){
        cout<<conn->toString()<<" has closed!"<<endl;
    }

private:
    ThreadPool _threadPool;
    TcpServer _server;
};