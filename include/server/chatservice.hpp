#ifndef CHATSERVICE_H
#define CHATSERVICE_H


#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include<mutex>
#include"usermodel.hpp"
#include"json.hpp"
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include "redis.hpp"

using json = nlohmann::json;

using namespace std;
using namespace muduo;
using namespace muduo::net;


//表示处理消息事件的回调方法类型  为什么写作单例模式呢
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp)>;   //msgid对应的消息回调
//聊天服务器业务类
class ChatService{
public:
      
      //获取单列对象的接口函数
      static ChatService* instance();

      //写一个login登录和regiter注册,给一个messageid映射一个事件回调
      //处理登录业务
      void login(const TcpConnectionPtr& conn, json& js, Timestamp time);

      //处理注册业务
      void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);      
      
      //一对一聊天业务
      void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

      //添加好友业务
      void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);

      //创建群组业务
      void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

      //加入群组业务
      void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

      //群聊天业务
      void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);


      //从redis消息队列中获取订阅的消息
      void handleRedisSubscribeMessage(int id, string msg);


      //处理注销业务
      void loginout(const TcpConnectionPtr& conn, json& js, Timestamp time);


      //获取消息对应的处理器
      MsgHandler getHandler(int msgid);  //为什么根据msgid得到的msgid就能确定这个消息处理器能够处理相对应的login事件还是reg事件

      //处理客户端异常退出
      void clientCloseException(const TcpConnectionPtr& conn);





      //服务器异常，业务重置方法
      void reset();


private:
      ChatService();
     
      //存储消息id和其对应的业务处理方法
      unordered_map<int,MsgHandler> _msghandlerMap;   //消息处理器的表  让id和相对应的消息处理事件对应

      //存储在线用户的通信连接
      unordered_map<int,TcpConnectionPtr> _userConnMap;

      //定义互斥锁，保证线程安全
      mutex _connMutex;

      UserModel _userModel;

      offlineMsgModel _offlineMsgModel;

      FrdModel _friendModel;
      GroupModel _groupmodel;

      //redis操作对象
      Redis _redis;
      


};

#endif