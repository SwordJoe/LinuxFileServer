#pragma once
#include<mysql/mysql.h>
#include<stdio.h>
#include<iostream>
#include<string>
#include<vector>
using namespace std;


class Crud
{
public:
    Crud(const char *host,const char *user,const char *passwd,const char *database)
    :_host(host),_user(user),_passwd(passwd),_database(database)
    {
        _conn=mysql_init(nullptr);
        if(nullptr==_conn){
            perror("mysql_init");
        }
        connect();
    }
    
    int query(const string &queryWord,vector<vector<string>> &ret);
    int insert(const string &insertWord);
    int update(const string &updateWord);

private:
    void connect();

private:
    MYSQL *_conn;
    const char *_host;
    const char *_user;
    const char *_passwd;
    const char *_database;
};

void Crud::connect(){
    MYSQL * ret=mysql_real_connect(_conn,_host,_user,_passwd,_database,0,nullptr,0);
    if(ret==nullptr){
        printf("Connect MySQL error:%s\n",mysql_error(ret));
        return;
    }
    else{
        cout<<"MySQL 连接成功"<<endl;
    }
}

int Crud::query(const string &queryWord,vector<vector<string>> &ret){
    int queryRet=mysql_query(_conn,queryWord.c_str());
    if(queryRet!=0){
        //printf("mysql_query error:%s\n",mysql_error(queryRet));
        perror("mysql_query===");
        return -1;
    }
    MYSQL_RES* RES=mysql_store_result(_conn);
    //printf("mysql_num_rows=%d\n",(int)mysql_num_rows(RES));

    MYSQL_ROW row=mysql_fetch_row(RES);
    if(row==nullptr){
        //cout<<"未查询到"<<endl;
        return 0;
    }
    else{
        do{
            vector<string> tmp;
            for(int i=0;i<(int)mysql_num_fields(RES);++i){
                printf("%8s",row[i]);
                if(row[i]==nullptr){
                    tmp.push_back("NULL");
                }
                else{
                    tmp.push_back(string(row[i]));
                }
                //tmp.push_back(string(row[i]));
            }
            cout<<endl;
            ret.push_back(tmp);
        }while(nullptr!=mysql_fetch_row(RES));
    }
    mysql_free_result(RES);
    return 1;
}

int Crud::insert(const string &insertWord){
    int queryResult=mysql_query(_conn,insertWord.c_str());
    if(queryResult!=0){
        printf("Insert error:%s\n",mysql_error(_conn));
        return -1;
    }
    else{
        int ret=mysql_affected_rows(_conn);
        if(ret==0){
            return 0;
        }
        else{
            return 1;
        }
    }
}

int Crud::update(const string &updateWord){
    int ret=mysql_query(_conn,updateWord.c_str());
    if(ret){ //错误，返回非0值
        printf("MySQL query error:%s\n",mysql_error(_conn));
        return -1;
    }
    else{
        int ret=mysql_affected_rows(_conn);
        if(ret){
            cout<<"update success,mysql_affected_rows:"<<ret<<endl;
            return 1;
        }
        else{
            cout<<"update fail,mysql_affected_rows:"<<ret<<endl;
            return -1;
        }
    }
}