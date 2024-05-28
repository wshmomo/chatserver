#include"db.hpp"
    
//数据库的配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

//初始化数据库连接
MySQL::MySQL(){
    _conn = mysql_init(nullptr);    //开辟一块资源空间

}

//释放数据库连接资源
MySQL::~MySQL(){
    if(_conn != nullptr){
        mysql_close(_conn);
    }

}

//连接数据库
bool MySQL::connet(){
    MYSQL* p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0); //存储连接工具的内存，server的ip地址，用户名，密码，所要连接的数据库名字，mysqlserver默认的端口号，后面两个参数不需要关注
    if(p != nullptr){

        mysql_query(_conn, "set names gbk");   //相当于让支持中文
        LOG_INFO<<"connext mysql success!";
    }

    else{
        LOG_INFO<<"connext mysql fail!";

    }

    return p;

}

//更新操作 sql语句中的insert update delete
bool MySQL::update(string sql){
    if(mysql_query(_conn, sql.c_str())){   
        LOG_INFO <<__FILE__<<":"<<__LINE__<<":"<<sql<<"更新失败";
    }
    return true;

}

//查询操作 针对sql的select
MYSQL_RES* MySQL::query(string sql){
    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO <<__FILE__<<":"<<__LINE__<<":"<<sql<<"查询失败";
    }

    return mysql_use_result(_conn);

}


MYSQL* MySQL::getConnection(){
    return _conn;
}