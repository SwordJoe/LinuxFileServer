#include"Client.hpp"
#include"ThreadPool.hpp"
#include"TcpConnection.hpp"
#include"Socket.hpp"
#include"md5.h"
#include<iostream>
#include<sstream>
#include<string>
#include<functional>
using namespace std;

void func(){
    Socket sock;
    TcpConnection conn(sock.fd());
    Connector connector;
    connector.Connect(sock.fd());

    string msg("hello");
    Train_t train(msg.size(),msg);
    conn.sendTrain(train);

}
int main(int argc,char *argv[])
{
    // char buff[50]={0};
    // Compute_file_md5(argv[1],buff);
    // cout<<buff<<endl;  
    
    // sleep(100);
    ThreadPool threadPool(8,10);
    threadPool.start();
    Client client("Download");
    client.connect();
    //sleep(100);
    //client.upload();
    client.Register_Login();
    string cmd,arg;
    while(1){
        string input;
        getline(cin,input);
        //cout<<"input:"<<input<<endl;
        int ret=client.handleCmd(input,cmd,arg);
        if(0==ret){
            cout<<client.name()<<":/"<<client.path()<<"> ";
            continue;
        }
        if(cmd=="ls"){      //查看当前目录
            if(arg!="null"){
                cout<<"请输入正确的命令参数"<<endl;
            }
            else{
                client.ls();
            }
        }
        else if(cmd=="cd"){     //目录跳转
            client.cd(arg);
        }
        else if(cmd=="pwd"){    //查看当前路径
            cout<<"/"<<client.path()<<endl;
        }
        else if(cmd=="mkdir"){  //创建目录
            client.mkdir(arg);
        }
        else if(cmd=="get"){    //下载文件
            //将Client类的get下载函数当作一个任务放到线程池中
            threadPool.addTask(bind(&Client::get,&client,arg));
            cout<<client.name()<<":/"<<client.path()<<"> ";
            continue;  
        }
        else if(cmd=="put"){    //上传文件
            //sleep(3);
            //一定要再client前面加上&,不然就是值传递，值传递会销毁临时对象，client中的主连接_mainTcpConn也会被销毁
            threadPool.addTask(bind(&Client::put,&client,arg));
            cout<<client.name()<<":/"<<client.path()<<"> "; 
            continue;   
        }
        cout<<client.name()<<":/"<<client.path()<<"> ";
    }
    

    
    //while(1);

    // Connector connector(8888,"139.196.153.114");
    // connector.Connect();
    // int cfd=connector.cfd();

    // Train_t train;
    // memset(&train,0,sizeof(train));

    // int datalen=0;
    // char buff[65536]={0};
    
    // //接收数据长度
    // recv(cfd,&datalen,4,0);
    // //接收文件名
    // recv(cfd,buff,datalen,0);

    // //创建同名文件
    // int fd=open(buff,O_CREAT|O_RDWR,0666);
    // //接收文件内容
    // while(1){
    //     memset(buff,9,sizeof(buff));
    //     recv(cfd,&datalen,4,0);
    //     if(0==datalen){
    //         break;
    //     }

    //     int ret=recv(cfd,buff,datalen,MSG_WAITALL);
    //     if(ret!=65536){
    //         cout<<"ret="<<ret<<endl;
    //     }
    //     //cout<<"ret="<<ret<<endl;

    //     write(fd,buff,ret);  //接收到ret个字节，就写入ret个字节
    // }
}