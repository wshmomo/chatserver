#include"chatserver.hpp"
#include "chatservice.hpp"
#include<functional>
#include<iostream>
#include<string>
#include "json.hpp"
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg)
   :_server(loop,listenAddr, nameArg), _loop(loop)
{
    //注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));


    //注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

    //设置线程数量
    _server.setThreadNum(4);   //说明有三个线程会去onMessage


}

//启动服务
void ChatServer::start(){
    _server.start();
}


//上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn){

        //断开连接
        if(!conn->connected()){

        }

        //客户断开连接  处理用户的异常退出，客户端比如ctrl c 中断它，网络连接异常中断，用户的退出需要让他变成offline
        if(!conn->connected()){
            ChatService::instance()->clientCloseException(conn);   //这块是对于客户端自己断开
            // ChatService::instance()->clientCloseException(conn);   //服务器自己断开，比如ctrl c 强制退出，根本没有机会去修改mysql中的state
            conn->shutdown();
        }

}

//上报读写相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp time)
{

    //收到消息了
    string buf = buffer->retrieveAllAsString();  //这代表把缓冲区的数据取出来

    //数据的反序列化，数据的解码
    json js = json::parse(buf);


    //达到目的：完全解耦网络模块的代码和业务模块的代码
    //通过js["msgid"]获取-》业务handler->conn js time   C++里面接口相当于抽象基类或者回调
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());    //js["msgid"].get<int>()将js["msgid"]转换成int类

    //回调消息绑定好的消息处理器，来执行相应地业务处理   
    msgHandler(conn,js,time);   //有个问题，为什么这块的回调函数一调用就开始执行chatservice.cpp里面的对应的相关业务呢，比如login 或者regiter



}

