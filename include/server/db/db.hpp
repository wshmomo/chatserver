#ifndef DB_H
#define DB_H

#include<muduo/base/Logging.h>
#include<mysql/mysql.h>
#include<string>

using namespace std;

class MySQL
{
public:

    //初始化数据库lianjie
    MySQL();

    //释放数据库连接资源
    ~MySQL();

    //连接数据库
    bool connet();

    //更新操作
    bool update(string sql);

    //查询操作
    MYSQL_RES *query(string sql);

    //获取连接
    MYSQL* getConnection();







private:
    MYSQL* _conn;    //看成和mysqlserver的一个连接
    
};



#endif