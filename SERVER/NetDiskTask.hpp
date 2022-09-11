#include<fcntl.h>
#include<sys/stat.h>
#include<unistd.h>
#include<signal.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<limits.h>
#include<stdlib.h>
#include"../ProtoMsg/Message.pb.h"
#include"TcpConnection.hpp"
#include"Crud.hpp"
#include"Train.hpp"
#include"Redis.hpp"
#include<iostream>
#include<string>
using namespace std;
using std::cout;

class EventLoop;
class NetDiskTask
{
    friend class EventLoop;
public:
    NetDiskTask(const string &msg,TcpConnectionPtr conn, Crud &crud,const string &path,Redis &redis)
    :_msg(msg)
    ,_conn(conn)
    ,_crud(crud)
    ,_rootDirPath(path)
    ,_redis(redis)
    //,_loop(loop)
    {}

    void Register(const string &userName,const string &passwd);     //注册
    void login(const string &userName,const string &passwd);        //登录
    void ls(const string &curDirPath);
    void cd(const string &dirPath);
    void mkdir(const string &curDirPath,const string &dirName);
    void put(Cmd cmd);
    void get(Cmd cmd);
    bool cookieJudge(string cookie);
    
private:
    bool searchSameFile(const string &curDirPath,const string &filePathName);
    void recvFile(string &md5,string &fileName,int fileSize,int breakPoint);
    void sendFile(int fileFd,int peerfd,off_t fileSize);
    string RandomStr(int num);
    

private:
    Crud &_crud;
    string _msg;
    TcpConnectionPtr _conn;
    string _rootDirPath;
    Redis &_redis;
    //EventLoop &_loop;
};

void NetDiskTask::Register(const string &userName, const string &passwd){
    string queryWord;
    queryWord=string("select * from UserInfo where name =")+string("'")+userName+string("';");

    vector<vector<string>> null;
    int ret=_crud.query(queryWord,null);

    if(1==ret){       //查询到，说明同名
        string response("用户已存在");
        Train_t train(response.size(),response);
        _conn->sendTrain(train);
    }
    else if(0==ret){    //未查询到，进行注册，即往数据库插入用户名+密码
        string msg("insert into UserInfo(name,pwd) values");
        msg=msg+string("('")+userName+string("','")+passwd+string("');");
        //cout<<"插入语句=============="<<msg<<endl;
        int ret=_crud.insert(msg);
        if(1==ret){
            string response("注册成功");
            cout<<response<<endl;
            Train_t train(response.size(),response);
            _conn->sendTrain(train);

            //为新注册用户建立一个家目录
            string path=_rootDirPath+string("/")+userName;
            int ret=::mkdir(path.c_str(),0775);
            if(-1==ret){
                perror("mkdir");
            }
        }
        else if(-1==ret){
            string response("注册失败");
            Train_t train(response.size(),response);
            _conn->sendTrain(train);
        }
    }
    ////_conn->resetEpoll();        //重置fd的监听模式
}

void NetDiskTask::login(const string &userName,const string &passwd){
    string queryWord;
    queryWord=string("select * from UserInfo where name =")+string("'")+userName+"'"+" and pwd='"+passwd+"';";
    vector<vector<string>> null;
    int ret=_crud.query(queryWord,null);
    LoginFeedBack feedBack;
    if(1==ret){
        // string response("登录成功");
        // Train_t train(response.size(),response);
        // _conn->sendTrain(train);
        feedBack.set_flag(true);
        string cookie=RandomStr(userName[0]);
        feedBack.set_cookie(cookie);
        string response=feedBack.SerializeAsString();
        Train_t train(response.size(),response);
        _conn->sendTrain(train);
        _conn->setCookie(cookie);
        _redis.set(cookie,to_string(_conn->fd()));    //将该用户的cookie值加入redis
    }
    else{
        // string response("用户名或密码错误");
        // Train_t train(response.size(),response);
        // _conn->sendTrain(train);

        feedBack.set_flag(false);
        string response=feedBack.SerializeAsString();
        Train_t train(response.size(),response);
        _conn->sendTrain(train);
    }
    ////_conn->resetEpoll();
}

void NetDiskTask::ls(const string &curDirPath){
    string path=_rootDirPath+curDirPath;    //将根目录与用户当前目录进行拼接
    string response;
    cout<<"path="<<path<<endl;
    DIR *pdir=opendir(path.c_str());
    if(nullptr==pdir){
        perror("pdir");
        return;
    }
    struct dirent *pdirent;
    while((pdirent=readdir(pdir))!=nullptr){
        if(strcmp(pdirent->d_name,".")==0 || strcmp(pdirent->d_name,"..")==0){
            continue;
        }
        string filePath=path+"/"+pdirent->d_name;   //获取该文件的绝对路径
        cout<<"filePath="<<filePath<<endl;
        struct stat statbuf;
        memset(&statbuf,0,sizeof(statbuf));
        stat(filePath.c_str(),&statbuf);
        string type(S_ISDIR(statbuf.st_mode)?"d":"f");  //文件类型，目录还是普通文件
        off_t size=statbuf.st_size;                     //文件大小
        cout<<"size="<<size<<endl;
        char buff[500]={0};
        sprintf(buff,"%s\t%ld\t\t\t\t%s\n",type.c_str(),size,pdirent->d_name);  
        response+=string(buff);
    }
    cout<<response;
    Train_t train(response.size(),response);
    _conn->sendTrain(train);
    //_conn->resetEpoll();
}

void NetDiskTask::cd(const string &dirPath){
    int pos=dirPath.find_first_of('/',1);
    string userName(dirPath,1,pos-1);       //从客户端传来的路径中提取出用户名，即"家目录"
    cout<<"用户名："<<userName<<endl;
    string path=_rootDirPath+dirPath;       //cd的绝对路径
    cout<<"跳转的绝对路径为"<<path<<endl;
    
    struct stat statbuff;
    stat(path.c_str(),&statbuff);
    if(S_ISDIR(statbuff.st_mode)){
        string msg("true");
        Train_t train(msg.size(),msg);
        _conn->sendTrain(train);
    }
    else{
        string msg("false");
        Train_t train(msg.size(),msg);
        _conn->sendTrain(train);
        //_conn->resetEpoll();        //重置文件描述符epoll监听模式
        return;
    }

    string str;
    //char *realpath(const char *path,char *resolved_path);
    char *ret=realpath(path.c_str(),nullptr);
    if(ret==nullptr){
        str="fail";
        cout<<"无该目录"<<endl;
    }
    else{
        cout<<"ret="<<ret<<endl;
        string tmp(ret);
        int idx=tmp.find(userName);
        if(idx==string::npos){  //在跳转后的路径中未找到用户名，说明跳转过头了
            str="";
        }
        else{
            //判断一下是否string数组越界
            int pos=idx+userName.size();
            if(pos>=tmp.size()){
                str=string(tmp,idx+userName.size());
            }
            else{
                str=string(tmp,idx+userName.size()+1);  
            }
        }
    }
    
    Train_t train(str.size(),str);
    _conn->sendTrain(train);
    //_conn->resetEpoll();
}

void NetDiskTask::mkdir(const string &curDirPath,const string &dirName){
    cout<<"curDirPath="<<curDirPath<<endl;
    cout<<"dirName="<<dirName<<endl;
    string path=_rootDirPath+curDirPath;
    DIR *pdir=opendir(path.c_str());
    struct dirent *pdirent;
    while((pdirent=readdir(pdir))!=nullptr){
        if(string(pdirent->d_name)==dirName){
            //发送目录重名消息给用户
            string str("same");
            Train_t train(str.size(),str);
            _conn->sendTrain(train);
            return;
        }
    }
    string dirPathName=_rootDirPath+curDirPath+dirName;
    int ret=::mkdir(dirPathName.c_str(),0775);
    if(-1==ret){
        perror("mkdir");
        return;
    }
    string str("success");
    Train_t train(str.size(),str);
    _conn->sendTrain(train);
     //_conn->resetEpoll();
}

//处理客户端上传文件的命令
void NetDiskTask::put(Cmd cmd){
    bool FLAG=false;
    cout<<"线程:"<<pthread_self()<<"在工作"<<endl;
    //接收文件md5+文件大小+绝对路径
    string dirPath=cmd.putinfo().curdirpath();      //当前目录
    string fileName=cmd.putinfo().filename();       //文件名
    string md5=cmd.putinfo().md5();                 //文件md5码
    int fileSize=cmd.putinfo().filesize();          //文件大小
    string filePathName=_rootDirPath+dirPath+fileName;  //拼接文件绝对路径名
    cout<<"filePahtName="<<filePathName<<endl;
    cout<<"fileSize="<<fileSize<<endl;
    cout<<"md5="<<md5<<endl;

    //判断当前目录下是否有同名文件
    string curDirPath=_rootDirPath+dirPath;
    bool sameNameFlag=searchSameFile(curDirPath,fileName);
    //查询数据库有相同md5值
    vector<vector<string>> record;
    string queryWord="select size,realpath from FileInfo where md5='";
    queryWord=queryWord+md5+"';";
    cout<<"queryWord="<<queryWord<<endl;
    int md5Flag=_crud.query(queryWord,record);

    if(sameNameFlag){ //该目录下有同名文件
        //有两种情况是单纯重名
        //1.数据库中并没有查找到上传文件的md5值
        //2.数据库中有上传文件的md5值，但是该md5的绝对路径并不是当前目录
        if(md5Flag==false || (md5Flag==true && filePathName!=record[0][1])){
            string msg("same");
            Train_t train(msg.size(),msg);
            _conn->sendTrain(train);
            //_conn->resetEpoll();        //记得重置fd的epoll监听模式
            return;         //直接返回，后面不用处理了
        }
        else{   //数据库中有md5值，且该md5的绝对路径就是当前目录，不是重名，是断点续传
            FLAG=true;
            string msg("notsame");
            Train_t train(msg.size(),msg);
            _conn->sendTrain(train);
        }
    }
    else{
        string msg("notsame");
        Train_t train(msg.size(),msg);
        _conn->sendTrain(train);
    }
    
    if(md5Flag){    //数据库中有相同md5码
        cout<<"有记录"<<endl;
        string realSize=record[0][0];       //保存在服务端的文件实际大小，可能是不完整的
        string realPath=record[0][1];       //保存在服务端的文件实际路径
        cout<<"realSize="<<realSize<<endl;
        cout<<"realPath="<<realPath<<endl;

        if(realSize!=to_string(fileSize)){     //文件实际大小与客户端文件大小不一致，断点续传
            //先告知客户端需要断点续传
            string msg("断点续传");         
            Train_t train_msg(msg.size(),msg);
            _conn->sendTrain(train_msg);

            //再告知客户端断点续传的偏移量
            Train_t train_breakPoint(realSize.size(),realSize);
            _conn->sendTrain(train_breakPoint);   
        
            //参数4表示数据库中是否已经有该md5,为true的话，则在接收完毕后更新数据库，否则就是插入一条新数据
            recvFile(md5,realPath,fileSize,atoi(realSize.c_str()));     

            if(FLAG==false){        //断点续传并不是在当前目录，所以要在当前目录建立硬链接
                string oldpath=realPath;        //被链接的文件路径
                string newpath=filePathName;    //链接过去的文件路径
                cout<<"oldpath="<<oldpath<<endl;
                cout<<"newpath="<<newpath<<endl;
                
                int ret=link(oldpath.c_str(),newpath.c_str());
                if(-1==ret){
                    perror("link");
                    return;
                }
                cout<<"创建硬链接成功"<<endl;
            }
        }
        else{       //md5码一样，实际大小也与客户端接发送过来的文件大小一样，直接秒传
            string msg("秒传");
            Train_t train(msg.size(),msg);
            _conn->sendTrain(train);

            if(FLAG==false){
                string oldpath=realPath;
                string newpath=filePathName;
                cout<<"oldpath="<<oldpath<<endl;
                cout<<"newpath="<<newpath<<endl;
                
                int ret=link(oldpath.c_str(),newpath.c_str());
                if(-1==ret){
                    perror("link");
                }
                cout<<"创建硬链接成功"<<endl;
            } 
        }
    }
    else{   //数据库中无相同md5码，普通传输
        cout<<"无记录,开始普通传输"<<endl;
        string msg("普通传输");
        Train_t train(msg.size(),msg);
        _conn->sendTrain(train);
        //进行接收前先立刻往数据库中插入该文件的信息
        char crudBuff[500]={0};
        string linkPath=filePathName;   //先保存原路径，后面可能要做链接
        int breakPoint=0;
        int flag=false;
        // cout<<"插入数据前先睡10秒..."<<endl;
        // sleep(10);
        // cout<<"两秒后开始工作"<<endl;
        // sleep(2);
        // cout<<"开始工作"<<endl;
        sprintf(crudBuff,"insert ignore into FileInfo(md5,size,realpath,refcnt,flag) values('%s',%d,'%s',%d,'%s')",md5.c_str(),0,filePathName.c_str(),1,"false");
        cout<<"插入语句为："<<crudBuff<<endl;
        int ret=_crud.insert(crudBuff);
        if(0==ret){     //插入0行，说明该md5码文件在数据库中已经有记录了，插入被忽略，重新查询，提取出真正的realPath
            //再重新查询，提取出真正的路径名
            vector<vector<string>> record;
            string queryWord="select size,realpath from FileInfo where md5='";
            queryWord=queryWord+md5+"';";
            //cout<<"queryWord="<<queryWord<<endl;
            int md5Flag=_crud.query(queryWord,record);
            breakPoint=atoi(record[0][0].c_str());
            filePathName=record[0][1];          //改变服务端接收数据的文件路径
            cout<<"已有数据，并不是普通接收，真正的路径在"<<filePathName<<endl;
            cout<<"新的breakPoint:"<<breakPoint<<endl;
            flag=true;

            string msg=to_string(breakPoint);
            Train_t train(msg.size(),msg);
            _conn->sendTrain(train);        //告知客户端偏移量
        }
        else if(1==ret){   //插入了一行，说明是真正的普通传输
            string msg("null");
            Train_t train(msg.size(),msg);
            _conn->sendTrain(train);    //告知客户端不用偏移
            cout<<"插入成功"<<endl;
        }
        else{
            cout<<"插入失败"<<endl;
        }

        recvFile(md5,filePathName,fileSize,breakPoint);        //该fileSize是客户端传过来的完整的文件大小
        if(flag==true){     //说明刚准备在本目录接收文件，其他用户也上传了该md5码文件，改变传输路径，同时建立硬链接
            int ret=link(filePathName.c_str(),linkPath.c_str());
            cout<<"真正的文件路径"<<filePathName<<endl;
            cout<<"链接文件路径"<<linkPath<<endl;
            if(-1==ret){
                perror("link");
            }
            else{
                cout<<"创建硬链接成功"<<endl;
            }
            
        }
    }
    //_conn->resetEpoll();        //重置fd的监听模式
}

//查询当前目录有无同名文件
bool NetDiskTask::searchSameFile(const string &curDirPath,const string &filePathName){
    DIR *pdir=opendir(curDirPath.c_str());
    if(nullptr==pdir){
        perror("pdir");
        return true;
    }

    struct dirent *pdirent;
    while((pdirent=readdir(pdir))!=nullptr){
        if(strcmp(pdirent->d_name,filePathName.c_str())==0){
            return true;
        }
    }
    return false;
}

//接收文件
void NetDiskTask::recvFile(string &md5,string &filePathName,int fileSize,int offset)
{   
    int cfd=_conn->fd();
    int realSize=offset,ret;
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
    off_t offSet=offset;
    while(1){
        ret=splice(cfd,0,pipefd[1],0,4096,0);
        splice(pipefd[0],0,fileFd,&offSet,ret,0);
        if(ret==0){
            break;
        }
        realSize+=ret;
    }

    // //接收完毕，查看接收后的文件大小，更新数据库记录
    // struct stat statbuf;
    // ret=stat(filePathName.c_str(),&statbuf);
    // int realSize=statbuf.st_size;
    
    char crudBuff[500]={0};
    memset(crudBuff,0,sizeof(crudBuff));
    string fullFlag(realSize==fileSize?"true":"false"); //文件是否完整
    //更新数据库文件表
    sprintf(crudBuff,"update FileInfo set size=%d,flag='%s' where md5='%s';",realSize,fullFlag.c_str(),md5.c_str());
    cout<<crudBuff<<endl;
    ret=_crud.update(crudBuff);
    if(1==ret){
        cout<<"更新成功"<<endl;
    }
    else{
        cout<<"更新失败"<<endl;
    }
    
    cout<<"接收完毕"<<endl;
    close(fileFd);      //关闭文件描述符
}


void NetDiskTask::get(Cmd cmd){
    string dirPath=cmd.getinfo().curdirpath();
    string fileName=cmd.getinfo().filename();

    string curPath=_rootDirPath+dirPath;    //当前目录
    string filePath=curPath+fileName;       //需要下载文件的绝对路径
    cout<<"curPath="<<curPath<<endl;
    cout<<"filePath="<<filePath<<endl;

    //判断当前目录有无该文件
    DIR *pdir=opendir(curPath.c_str()); 
    struct dirent *pdirent;
    bool flag=false;
    while((pdirent=readdir(pdir))!=nullptr){
        if(pdirent->d_name==fileName){
            flag=true;
            break;
        }
    }

    if(flag==false){
        string msg("false");
        Train_t train(msg.size(),msg);
        _conn->sendTrain(train);
        //_conn->resetEpoll();        //重置fd的监听模式
        return;
    }
    else{
        string msg("true");
        Train_t train(msg.size(),msg);
        _conn->sendTrain(train);
        cout<<"有该文件"<<endl;
    }

    
    int fileFd=open(filePath.c_str(),O_RDONLY);
    if(-1==fileFd){
        perror("open");
        return;
    }
    else{
        cout<<"打开"<<filePath<<"成功"<<endl;
    }
    struct stat statbuff;
    stat(filePath.c_str(),&statbuff);
    off_t fileSize=statbuff.st_size;
    //将文件大小发送给客户端
    string msg=to_string(fileSize);
    Train_t train(msg.size(),msg);
    _conn->sendTrain(train);

    int peerfd=_conn->fd();
    sendFile(fileFd,peerfd,fileSize);
}

void NetDiskTask::sendFile(int fileFd,int peerfd,off_t fileSize){
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
    while(1){
        int ret=splice(fileFd,0,pipefd[1],0,4096,0);
        splice(pipefd[0],0,peerfd,0,ret,0);
        if(ret==0){
            break;
        }
    }
    cout<<"发送完毕"<<endl;
}


string NetDiskTask::RandomStr(int num){
	string str;
	str.resize(10);
	int flag;
	srand(time(nullptr));
	for(int i=0;i<10;++i){
		flag=(num+rand())%3;
		switch(flag){
			case 0:str[i]=rand()%26+'a';break;
			case 1:str[i]=rand()%26+'A';break;
			case 2:str[i]=rand()%10+'0';break;
		}
	}
	return str;
}

bool NetDiskTask::cookieJudge(string cookie){
    //return false;
    char buff[128]={0};
    //int ret=recv(_conn->fd(),buff,sizeof(buff),MSG_PEEK);
    if(cookie=="NULL"){
        string msg("请进行登录操作");
        Train_t train(msg.size(),msg);
        _conn->sendTrain(train);
    }
    else{
        cout<<"cookie="<<cookie<<endl;
        string ret=_redis.get(cookie);
        if(ret!="NULL"){
            string msg("跳过登录");
            Train_t train(msg.size(),msg);
            _conn->sendTrain(train);
            _conn->resetEpoll();
        }
    }
    //_conn->resetEpoll();
}