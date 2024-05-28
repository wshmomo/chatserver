#include"chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
#include<map>
#include"user.hpp"

// #include<muduo/net/TcpServer.h>


// #include<iostream>

//获取单例对象的接口函数
ChatService* ChatService::instance(){
     static ChatService service;
     return &service;

}

//注册消息以及对应的回调操作

ChatService::ChatService(){

    //用户基本业务管理相关事件处理回调注册
    _msghandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msghandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});
    _msghandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msghandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msghandlerMap.insert({ADD_FRIEND_MES,std::bind(&ChatService::addFriend,this,_1,_2,_3)});

    //群组业务管理相关事情处理回调注册
    _msghandlerMap.insert({GREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msghandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msghandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    

    //连接redis服务器
    if(_redis.connect()){

        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }


}


MsgHandler ChatService::getHandler(int msgid){

    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msghandlerMap.find(msgid);
    if(it == _msghandlerMap.end()){
    
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp){
            LOG_ERROR << "msgid:"<<msgid<<"can not find handler!";

        };
    }

    else{
        return _msghandlerMap[msgid];
    }

    
}


//处理登录业务  ORM 业务层操作的都是对象，看不到mysql语句的
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time){

    LOG_INFO<<"do login service!!!";
    int id = js["id"].get<int>();    //有个问题客户端是怎么知道发送的这个id刚好就是在服务端记录的Id

    User user = _userModel.query(id);
    if(user.getID() == id && user.getPwd() == js["password"]){
        if(user.getState() == "online"){
            //该用户已经登陆，不允许重复登录
           json respond;
           respond["msgid"] = LOGIN_MSG_ACK;
           respond["erron"] = 2;
           respond["errmsg"] = "this account is using, input another!";
           conn->send(respond.dump());

        }
        else{

            //登录成功，记录用户的连接信息
            {
                lock_guard<mutex> lock(_connMutex);  //加锁
               _userConnMap.insert({id,conn});   //要考虑这个线程安全的问题，因为有可能有多个线程一起访问onMessage，此时多个用户一起登录，这个数据可能会出现前后不一致的问题
                                              //C++容器没有考虑多线程并发出现的线程安全问题
            }   //锁的粒度要小，要不然很长解一次锁，那多线程并行还有什么意义，相当于单线程了

           //id用户登录成功后，想redis订阅channel(id);
            _redis.subscribe(id);

            //登录成功,更新用户的状态信息 state offline->online
            user.setState("online");          //数据库的并发操作不需要考虑，这是Mysql考虑的
            _userModel.updateState(user); 


            json respond;                   //局部变量，每个线程的栈都是隔离的，不冲突的
            respond["id"] = user.getID();
            respond["name"] = user.getName();
            respond["msgid"] = LOGIN_MSG_ACK;
            respond["erron"] = 0;

            //查询该用户是否有离线消息
            vector<string> vecMsg = _offlineMsgModel.query(id);
            if(!(vecMsg.empty())){
                //让服务器推送暂存的消息
                respond["offlinemsg"] = vecMsg;  //将所有的消息都推送到里面,可以直接跟容器直接序列化
                //删除服务器端存储的消息
                _offlineMsgModel.remove(id);
                // conn->send(respond.dump());
                // conn->send(respond.dump());
            }

            //查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty()){
                map<string, string> friends;
                for(User &user :userVec){
                    friends.insert({user.getName(),user.getState()});
                   
                }
                respond["friends"] = friends;
            }

            //查询该用户的群组信息并返回
            vector<Group> groupVec = _groupmodel.queryGroups(id);
            if(!groupVec.empty()){
                vector<string> vec2;
                for(Group &group : groupVec){
                    json js;
                    js["groupid"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();
                    // js["groupmembers"] = group.getUser();
                    vector<string> userV;
                    for(GroupUser & user : group.getUser()){
                        json userjs;
                        userjs["id"] = user.getID();
                        userjs["name"] = user.getName();
                        userjs["state"] = user.getState();
                        userjs["role"] = user.getRole();
                        userV.push_back(userjs.dump());


                    }
                    js["user"] = userV;
                    vec2.push_back(js.dump());
                }
                respond["group"] = vec2;

            }



            conn->send(respond.dump());

        }


    }

    else{
        //该用户不存在，用户存在但是密码错误，登录失败（两种情况）
        json respond;
        respond["msgid"] = LOGIN_MSG_ACK;
        respond["erron"] = 1;
        respond["errmsg"] = "name or password is invalid!";
        conn->send(respond.dump());

    }

}

//处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time){

    LOG_INFO<<"do reg service!!!";
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if(state){
        // LOG_FIFO<<"注册成功";
        json respond;
        respond["msgid"] = REG_MSG_ACK;
        respond["errno"] = 0;  //0表示响应成功， 1表示失败
        respond["id"] = user.getID();
        conn->send(respond.dump());
    }




}


//一对一聊天业务，这里是跟谁都能聊天，按理说只能跟你的好友聊天，所以还需要修改一下，那么就需要定义一个好友的列表
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time){

    int toid = js["to"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()){
            //toid在线，转发消息
            it->second->send(js.dump());   //服务器直接把从id用户的信息直接推送给toid用户

            return;

           
        }
        

    }

    //查询toid是否在线，如果在线说明他在其他服务器登录着
    User user = _userModel.query(toid);
    if(user.getState() == "online"){
        _redis.publish(toid,js.dump());
        return ;
    }

    //toid不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump());




}


//添加好友业务
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int frdid = js["friendid"].get<int>();
    int userid = js["id"].get<int>();
    _friendModel.insert(frdid,userid);  //感觉这里有点问题，你需要存在吧



}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
    Group group(-1,groupname,groupdesc);
    // group.setName(groupname);
    // group.setDesc(groupdesc);

    bool isCreateGroup = _groupmodel.createGroup(group);

    if(isCreateGroup){

        //你本人创建的群组，你自己需要加入到群组中
        _groupmodel.addGroup(userid,group.getId(),"creator");
        // json respond;
        // respond["GREATE_GROUP_MSG"] = REG_MSG_ACK;
        // respond["errno"] = 0;  //0表示响应成功， 1表示失败
        // respond["groupid"] = group.getId();
        // conn->send(respond.dump());

    }

}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time){
    // string 
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupmodel.addGroup(userid,groupid,"normal");

}

//群聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int groupid = js["groupid"];
    int userid = js["id"];
    vector<int> membersid = _groupmodel.queryGroupUsers(userid,groupid);
    lock_guard<mutex> lock(_connMutex);
    for(int memberid : membersid){

            auto it = _userConnMap.find(memberid);
            if(it != _userConnMap.end()){
                //toid在线，转发消息
                it->second->send(js.dump());   //服务器直接把从id用户的信息直接推送给toid用

            
            }
            else{      

                //查询toid是否在线
                User user = _userModel.query(memberid);
                if(user.getState() == "online"){

                    _redis.publish(memberid,js.dump());
                }
                else{    

                    //toid不在线，存储离线消息
                   _offlineMsgModel.insert(memberid,js.dump());
                }



            }
        
    

    }



    

}



void ChatService::handleRedisSubscribeMessage(int userid, string msg){


    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if( it != _userConnMap.end()){  //redis服务器直接把消息推送给userid
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    _offlineMsgModel.insert(userid,msg);

}

void ChatService::loginout(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end()){
            _userConnMap.erase(it);
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    if(userid != -1){
        User user;
        user.setID(userid);
        user.setState("offline");
        _userModel.updateState(user);

    }

}


//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr& conn){
    //异常退出也需要注意线程安全问题，因为你需要修改在线的用户，只要修改那个值就需要加锁操作
    User user;
    {
        lock_guard<mutex> lock(_connMutex);  //加锁
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); it++){
            if(it->second == conn){
                //从map表删除用户的连接信息
                user.setID(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    } 

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getID());

    //更新对应id的用户信息
    if(user.getID()!=-1){
        user.setState("offline");
        _userModel.updateState(user);

    }
  
}



//服务器异常，业务重置方法
void ChatService::reset(){
    //把所有用户的状态online，设置为offline
    _userModel.resetState();

}

