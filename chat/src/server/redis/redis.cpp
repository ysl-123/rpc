#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis()
    : _publish_context(nullptr), _subcribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subcribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}

// 向redis指定的通道channel发布消息 这个实际上就是给你要发消息的那个玩意pubulish
bool Redis::publish(int channel, string message)
{   
    //这里为什么可以使用publish因为这里的命令一执行就可以回复不会进入等待状态
    /*redisCommand实际上是
    redisAppendCommand  把 Redis 命令（如 PUBLISH channel msg）按照 Redis 协议（RESP）格式化为字节流，然后存入上下文（redisContext）的 输出缓冲区（obuf） 中，不立即发送到网络。
    +redisBufferWrite  将本地缓冲区的命令通过网络发送到 Redis 服务器
    +redisGetReply   阻塞等待 Redis 服务器的响应，并解析为结构化结果
    */
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
     /*  
    redisAppendCommand  它会把SUBSCRIBE %d命令按照 Redis 协议格式，写入_subcribe_context的输出缓冲区（obuf），但不会立即发送到网络。
    +redisBufferWrite  把输出缓冲区（obuf）中的数据通过 socket 发送给 Redis 服务器
    +redisGetReply   等待 Redis 推送的频道消息
    */
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收 Redis 发布/订阅通道的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    
    // 一直阻塞等待：只要订阅连接正常，就循环接收消息
    while (REDIS_OK == redisGetReply(this->_subcribe_context, (void **)&reply))
    {   //当客户端通过 SUBSCRIBE 命令订阅某个频道后，Redis 服务器在该频道有新消息时，会向订阅者推送固定结构的消息，
        // 订阅消息格式一般是 ["message", "channel", "content"]
        // 这里检查消息格式是否正确
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 调用业务层注册的回调函数，把通道号和消息内容上报上去
            // reply->element[1] 是通道号（字符串），需要 atoi 转成 int
            // reply->element[2] 是消息内容（字符串）
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        // 释放 redisReply 结构体，防止内存泄漏
        freeReplyObject(reply);
    }

    // redisGetReply 返回不是 REDIS_OK 时（例如连接断了），跳出循环
    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}


void Redis::init_notify_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler = fn;
}