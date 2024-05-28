#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include<iostream>
#include<vector>
#include"user.hpp"
using namespace std;


//维护好友信息的操作接口方法
class FrdModel{
public:
    //添加好友信息
    void insert(int frdid,int userid);


    //删除好友信息
    void update(int frdid);

    //查询好友信息 返回用户好友列表
    vector<User> query(int userid);

private:
};


#endif