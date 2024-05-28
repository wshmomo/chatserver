#include"chatserver.hpp"
#include"chatservice.hpp"
#include<signal.h>
#include<iostream>
 using namespace std;

//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int){

   ChatService::instance()->reset();
   exit(0);
}

int main(int argc, char **argv){
    if(argc < 3){
        cerr <<"command invalid! example: ./ChatServer 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    signal(SIGINT,resetHandler);   //这里的SIGNIT是指在服务器处按下 ctrl +c 为什么这里不写在网络断的连接处呢？因为一旦发生异常全部断开，需要处理所有的连接，而且这不属于连接，这属于服务器问题
                                   //如果在客户端处按下ctrl +c并不会触发这个事件

    EventLoop loop;
   //  InetAddress addr("127.0.0.1",6000);
    InetAddress addr(ip,port);

    ChatServer server(&loop, addr,"ChatServer");

    server.start();
    loop.loop();

    return 0;

 }