#include<string>
#include<iostream>
using namespace std;

class Put
{
public:
    Put(string &fileName,string &md5):_md5(md5),_fileName(fileName){}
    void put();
private:
    string _md5;
    string _fileName;
};