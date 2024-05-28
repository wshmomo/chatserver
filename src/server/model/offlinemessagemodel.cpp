#include"offlinemessagemodel.hpp"
#include"db.hpp"


//存储用户的离线消息
bool offlineMsgModel::insert(int id, string msg){
    
    char sql[1024] ={0};
    sprintf(sql,"insert into offlinemessage(userid, massage) values(%d,'%s')",id,msg.c_str());
    MySQL mysql;
    if(mysql.connet()){
        if(mysql.update(sql)){
            return true;
        }
    }
    return false;

    

}

//删除用户的离线消息
bool offlineMsgModel::remove(int userid){
    char sql[1024] = {0};
    sprintf(sql,"delete from offlinemessage where userid = %d",userid);
    MySQL mysql;
    if(mysql.connet()){
        if(mysql.update(sql)){
            return true;
        }

    }

    return false;

}

//查询用户的离线消息
vector<string> offlineMsgModel::query(int userid){
    
    vector<string> vecMsg;
    char sql[1024] = {0};
    sprintf(sql,"select massage from offlinemessage where userid = %d",userid);

    MySQL mysql;
    if(mysql.connet()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                vecMsg.push_back(row[0]);
            }
            mysql_free_result(res);
            // return vecMsg;
            

        }

        
    }

    return vecMsg;

}