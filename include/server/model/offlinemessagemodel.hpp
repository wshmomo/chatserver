#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include<string>
#include<vector>
// #include<map>
using namespace std;
//提供离线消息表的操作接口方法
class offlineMsgModel{
public:
    //存储用户的离线消息
    bool insert(int id, string msg);

    //删除用户的离线消息
    bool remove(int userid);

    //查询用户的离线消息
    vector<string> query(int userid);

private:
};



#endif