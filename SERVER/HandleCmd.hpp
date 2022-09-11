#include"../ProtoMsg/Message.pb.h"
#include<string>
#include<iostream>
using namespace std;

class HandleCmd
{
public:
    HandleCmd(const string &msg):_msg(msg){}
    int parse(const string &cmdMsg);
private:
    string _msg;
};

int HandleCmd::parse(const string &cmdMsg){
    Cmd cmd;
    cmd.ParseFromString(_msg);
    return cmd.cmdid();
    // switch(cmdid){
    //     case 1:{    //Register
    //         // string userName=cmd.registerinfo().username();
    //         // string passwd=cmd.registerinfo().passwd();
    //         // cout<<"cmdid="<<cmdid<<endl;
    //         // cout<<"userName："<<userName<<endl;
    //         // cout<<"passwd: "<<passwd<<endl;
            
    //         break;
    //     }     
    //     case 2:{    //login

    //     }
    // }
}