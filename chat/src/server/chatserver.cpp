#include "chatserver.hpp"
#include "chatservice.hpp"
#include <string>
#include "json.hpp"
#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开链接  这种方式是只有客户端主动断开才行，ctr+c是对服务器进程的 SIGINT，触发不了
    if (!conn->connected())
    {  
        ChatService::instance()->clientCloseException(conn);
        //conn->shutdown(); 应该不能有这句吧，你都断开了你还发起什么四次挥手呢
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 函数作用好像是从缓冲区（Buffer）中取出当前所有可读数据，并以 std::string 形式返回
    // 目前返回为string类型能够理解因为是json字符串，至于给muduo传入时传的是什么后面看看
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);
    // 达到的目的: 完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取=>业务handler=>conn js time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js, time);
}

