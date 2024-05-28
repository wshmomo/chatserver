#include"usermodel.hpp"
#include"db.hpp"
#include<iostream>

using namespace std;

bool UserModel::insert(User& user){
    //1.组装sql语句
    char sql[1024] ={0};
    sprintf(sql,"insert into user(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());   //忘sql里面插入字符串

    MySQL mysql;
    
    if(mysql.connet()){
        if(mysql.update(sql)){
            //获取插入成功的用户数据生成的主键id
            user.setID(mysql_insert_id(mysql.getConnection()));  //这里的mysql_insert_id(MYSQL* mysql),返回一个id
            return true;

        }
    }

    return false;


}


User UserModel::query(int id){
    //1.组装sql语句
    char sql[1024] ={0};
    sprintf(sql,"select * from user where id = %d",id);   //忘sql里面插入字符串

    MySQL mysql;
    
    if(mysql.connet()){
        // if(mysql.query(sql)){
        //     //获取插入成功的用户数据生成的主键id
        //     user.setID(mysql_insert_id(mysql.getConnection()));  //这里的mysql_insert_id(MYSQL* mysql),返回一个id
        //     return true;

        // }
        MYSQL_RES* res = mysql.query(sql);  //因为这块是指针，所以mysql_free_result(res);需要释放内存
        if(res != nullptr){
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr){
                User user;
                user.setID(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);  
                return user;
            }
        }
    }

}



bool UserModel::updateState(User user){
    //1.组装sql语句
    char sql[1024] ={0};
    sprintf(sql,"update user set state = '%s' where id = %d",user.getState().c_str(), user.getID());   //忘sql里面插入字符串

    MySQL mysql;
    
    if(mysql.connet()){
        if(mysql.update(sql)){
            return true;
        }

    }

    return false;


}

//重置用户的状态信息
void UserModel::resetState(){

    char sql[1024] = "update user set state = 'offline' where state = 'online'";
    MySQL mysql;
    if(mysql.connet()){
        mysql.update(sql);
    }

}


