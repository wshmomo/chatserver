#include<thread>
#include "redis.hpp"

/*同步redis的发布订阅的模版*/

/*异步redis的发布订阅也可以支持，但是更加复杂，需要进行事件循环去绑定，用lbenvent1.。。。去绑定*/

Redis::Redis()
    :_publish_context(nullptr), _subcribe_context(nullptr)
{

}

Redis::~Redis(){
    if(_publish_context != nullptr){
        redisFree(_publish_context);
    }

    if(_subcribe_context != nullptr){
        redisFree(_subcribe_context);
    }

}

//连接redis服务器
bool Redis::connect(){
    //负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1",6379);  //redis默认的端口可以去查询吗，查询出来的结果是6379
    if(_publish_context == nullptr){
        cerr << "connect redis faild!" <<endl;
        return false;


    }

    //负责subscribe订阅消息的上下文连接
    _subcribe_context = redisConnect("127.0.0.1",6379);
    if(_subcribe_context == nullptr){
        cerr << "connect redis faild!" <<endl;
        return false;
    }
    //订阅完了就结束了，不能霸占ChatServer1，他还要干其他事情

    //在单独的线程中，监听通道上的事件，有消息给业务层进行上报,这里全局connect一次，所以这里只其一个线程
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    std::cout << "connect redis-server success!" <<endl;
    return true;
    

}

//向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message){
    //redisCommand先把你要发送的命令（"PUBLISH %d %s",channel,message.c_str()）缓存到本地，然后发送到redisServer上，这里不会阻塞是因为publish这个命令一执行就直接返回了
    redisReply *reply = (redisReply *)redisCommand(_publish_context,"PUBLISH %d %s",channel,message.c_str());  //这里的意思就是发一个命令就直接过去了
    if(reply == nullptr){
        cerr << "publish command failded!" <<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;

}

//向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel){

    //SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    //通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    //只负责发送命令，不阻塞接收redis server响应消息， 否则和notifyMsg线程抢占响应资源
    if(redisAppendCommand(this->_subcribe_context,"SUBSCRIBE %d", channel) == REDIS_ERR){  //命令组装好写到缓存
        cerr << "subscribe command failed!" <<endl;
        return false;
    }

    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subcribe_context,&done) == REDIS_ERR){  //把命令从本地的缓存发送到redisServer上
            cerr << "subscribe command failed!" <<endl;
            return false;

        }
    }

    //redisGetReply  redisCommand这个函数包含redisAppendCommand，redisBufferWrite，redisGetReply这三函数的组合，这里由于堵塞问题不用redisCommand，也就是说我们不调用redisGetReply，不用阻塞等回复

    return true;

}

//向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel){

    if(redisAppendCommand(this->_subcribe_context,"UNSUBSCRIBE %d", channel) == REDIS_ERR){  //命令组装好写到缓存
        cerr << "unsubscribe command failed!" <<endl;
        return false;
    }

    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while(!done){
        if(redisBufferWrite(this->_subcribe_context,&done) == REDIS_ERR){  //把命令从本地的缓存发送到redisServer上
            cerr << "unsubscribe command failed!" <<endl;
            return false;

        }
    }

   
    return true;

}


//在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message(){ //循环读取
    redisReply *reply = nullptr;
    while(redisGetReply(this->_subcribe_context,(void **)&reply) == REDIS_OK){
        //订阅收到的消息是一个带三元素的数组
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            //给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>>>>>>>>>>>>>>observer_channel_message quit <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" <<endl;

}

//初始化向业务层上报通道消息的回调对象
void  Redis::init_notify_handler(function<void(int,string)> fn){

    this->_notify_message_handler = fn;

} 