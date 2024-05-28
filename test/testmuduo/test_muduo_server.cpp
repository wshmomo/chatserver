/*muduo网络库给用户提供了两个主要的类
TcpServer:用于编写服务器程序的
TcpClient:用于编写客户端程序的

epoll+线程池
好处：能够把网络I/O的代码和业务代码区分开
                         用户的连接断开    用户的可读可写事件
*/

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
#include<string>
using namespace std;
using namespace placeholders;
/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置合适的服务端线程数量，muduo库会自己划分I/O线程和worker线程
*/

class ChatServer{
public:
    ChatServer(muduo::net::EventLoop* loop,        //事件循环
              const muduo::net::InetAddress& listenAddr,   //IP+Port
              const string& nameArg)              //服务器名称
              :_server(loop,listenAddr,nameArg)
              ,_loop(loop)
    {
        //给服务器注册用户连接的创建和断开回调
        //这里回调函数定义typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;注意std::function<void (const TcpConnectionPtr&)表示一个函数，其形参是const TcpConnectionPtr&
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));   //人家的callback都是没有返回值，一个形参变量，但是现在是一个成员方法，有个this指针，这样就跟callback类型不同了，需要用绑定器绑定一下

       //给服务器注册用户读写事件回调
       _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));


       //设置服务器端的线程数量 1个I/O线程 3个worker线程，笔记本上的那个图
       _server.setThreadNum(4);  



    }

    //开启事件循环
    void start(){
        _server.start();
    }


private:
    //专门处理用户的连接创建和断开  epoll listenfd accept
    void onConnection(const muduo::net::TcpConnectionPtr& conn){
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"state::online"<<endl;
        }
        else{
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"state::offline"<<endl;
            conn->shutdown();  //close(fd)
            //_loop->quit();
        }


    }  //这是成员方法，为了调用成员变量，但是有个问题，setConnectionCallback

    //专门处理用户的读写事件
    void onMessage(const muduo::net::TcpConnectionPtr &com,  // 连接
                  muduo::net::Buffer *buffer,                   //缓冲区
                  muduo::Timestamp  time)    //时间信息
    {
        string buf = buffer->retrieveAllAsString();  //将接收的数据全部放到字符串当中
        cout<<"recv data:"<<buf<<"time:"<<time.toString()<<endl;
        com->send(buf);



    }
    muduo::net::TcpServer _server;  
    muduo::net::EventLoop *_loop;


};


int main(){

    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");
    server.start();
    loop.loop();  // epoll_wait以阻塞方式等待新用户连接，已连接用户时间的操作

    return 0;
}