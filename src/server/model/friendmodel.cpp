#include"friendmodel.hpp"
// #include<iostream>
#include"db.hpp"


// using namespace std;

//存储好友信息
void FrdModel::insert(int frdid,int userid){
    char sql[1024] = {0};
    sprintf(sql,"insert friend(userid, friendid) values(%d,%d)",userid,frdid);
    MySQL mysql;
    if(mysql.connet()){
        mysql.update(sql);
    }

}


//删除好友信息
void FrdModel::update(int frdid){
    char sql[1024] = {0};
    sprintf(sql,"delete from friend where frdid = %d",frdid);
    MySQL mysql;
    if(mysql.connet()){
        mysql.update(sql);
    }

}

//查询好友信息
vector<User> FrdModel::query(int userid){
    char sql[1024] ={0};
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d",userid);
    MySQL mysql;
    vector<User> vecFrd;
    if(mysql.connet()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row= mysql_fetch_row(res)) != nullptr){
                User user;
                user.setID(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vecFrd.push_back(user);
                

            }
            mysql_free_result(res);
            return vecFrd;

        }
        
        // while((row= mysql_fetch_row(res)) != nullptr){
        //     vecFrd.push_back(atoi(row[0]));
        // }
    }

    return vecFrd;

}

