#include "rpcprovider.h"
#include <string>
#include "mprpcapplication.h"
#include <functional>
#include <google/protobuf/descriptor.h>
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"
/*
service_name => service描述
                => service* 记录服务对象
method_name => method方法对象
*/

void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;
    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务的名字   真的离谱竟然是UserService继承的UserServiceRpc这是他的服务的名字
    std::string service_name = pserviceDesc->name();
    // 获取服务对象service的方法的数量
    int methodCnt = pserviceDesc->method_count();

    LOG_INFO("service_name:%s", service_name.c_str());
    for (int i = 0; i < methodCnt; ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        LOG_INFO("method_name:%s", method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

void RpcProvider::Run()
{
    // 我感觉可以直接写成MprpcApplication::GetConfig()
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // 感觉是rpcserverport
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);

    // 创建TcpServer对象  因为这里初始化的m_eventLoop只要定义不就是可以直接构造函数了
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定连接回调和消息读写回调方法
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    //这个调用就是客户端与服务器建立连接由tcpconnection管理，里面有收到客户端发送消息时产生handleread回调，读完客户端发送完消息就进行message的回调
    //muduo库里这个回调是这样的messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);实际上就是等待caller发来请求等待处理和muduo一样message回调只是后面调用
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));

    // 设置muduo库的线程数量
    server.setThreadNum(5);
    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    //
    ZkClient zkCli;
    zkCli.Start();
    // service_name为永久性节点    method_name为临时性节点
    for (auto &sp : m_serviceMap)
    {
        // /service_name
        std::string service_path = "/" + sp.first;//比如就是  /userservicerpc
        //这里data为nullptr也就是直接创建一个节点，永久
        zkCli.Create(service_path.c_str(), nullptr, 0);//没写state就是默认0，也就是永久性节点
        for (auto &mp : sp.second.m_methodMap)
        {
            //      /userservicerpc/login
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            //在上面/userservicerpc这个目录里在创建一个临时   ZOO_EPHEMERAL是宏1，也就是临时节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }
    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    // 回头看下这个回调咋触发的，在muduo中
    if (!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();
    }
}

/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
service_name method_name args    定义proto的message类型，进行数据头的序列化和反序列化
但是如果传递的时候就直接UserServiceLoginzhang这样没办法区分折磨长一串属于三种的哪一种
                                 service_name method_name args_size所以要给他们标注大小
16 UserServiceRPCLogin5zhangsan 
header_size(4个字节固定，整数也不会超过4字节) + header_str + args_str

[header_size] [service_name] [method_name] [args_size] [args_str]
|--4字节 header_size--|--序列化后的RpcHeader--|--序列化后的args--|
[00 00 00 24] [....RpcHeader序列化数据....] [....args序列化数据....]
然后RpcHeader里有service_name    method_name  args_size
然后最后的才有是存的数据

*/
// 已建立连接用户的读写事件的回调，如果远程有一个rpc服务的调用请求，那么onmessage就会响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp)
{
    // 网络上接受的远程rpc调用请求的字符流  Login args
    // Muduo 网络库 里 Buffer 类的一个成员函数，用来把缓冲区里的所有数据取出来，并转换成一个 std::string 返回。
    std::string recv_buf = buffer->retrieveAllAsString();
    // 从字符流中读取前4个字节的内容，不用疑惑为啥读取四个，前 4 个字节的设置是在 mprpcchannel.cc 的 CallMethod 函数中显式处理
    uint32_t header_size = 0;
    recv_buf.copy((char *)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    // 从偏移量4开始截取header_size长度
    // recv_buf 本身完全不变，只是从它里面拷贝出一部分内容生成了一个新的字符串 rpc_header_str。

    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    // ParseFromString把序列化后的二进制字符串 data 解析（反序列化）成一个 Protobuf 消息对象。直接存进去rpcHeader里了
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        // 数据头反序列化失败
        std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 打印调试信息
    std::cout << "=====================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "=====================================" << std::endl;

    // 获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        std::cout << service_name << " is not exist!" << std::endl;
        return;
    }
    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }

    google::protobuf::Service *service = it->second.m_service;      // 获取service对象 new UserService
    const google::protobuf::MethodDescriptor *method = mit->second; // 获取method对象 Login

    // 1. 创建请求对象，并反序列化远端发来的请求参数（空的）
    // 所有 .proto 文件中定义的消息类型（如 LoginRequest, LoginResponse）最终都会继承自google::protobuf::Message
    //这个返回的是属于当前的method所属的service里面request的形式，毕竟有userservice还要friendservice呢
    //这里为什么不能直接google::protobuf::Message *request这样，是因为具体请求类型不知道比如loginrequest registerrequest
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str)) // 将args_str反序列化写入request
    {
        std::cout << "request parse error, content:" << args_str << std::endl;
        return;
    }
    // 2. 创建响应对象（空的）
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    // 一个作用就是就是response序列化后发给对方，但是done实际存储着前面这一部分的行为，
    //要在下面执行完method方法后，method方法里有一个done->run()就执行了，也就是相当于在这部分才开始进行对他的回调
    // 甚至response本来是空的，写入也是发生在method方法执行函数里面，执行完后，会写入success，然后存进去，然后done->run()
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                                                    const muduo::net::TcpConnectionPtr &,
                                                                    google::protobuf::Message *>(this,
                                                                                                 &RpcProvider::SendRpcResponse,
                                                                                                 conn, response);//千万注意这里response是指针也就是后面修改他也能接收到

    // 首先执行request请求里的method，然后method里的函数比如login会执行完后，写入reponse，然后done->run()
    // 这里千万注意调用的不是mprpcchannel的callmethod，就是最初的虚函数里的，连头文件都没有引用怎么可能是
    // userservice里不是有callmethod方法会进行执行，然后我实现了所谓的userservicerpc继承userservice，
    //实现了他的method方法，继承callmethod，rpcprovider只是进行分发我目前的service还是属于派生类的，所以callmethod只是传参不一样
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str)) // response进行序列化
    {
        // 序列化成功后，通过网络把rpc方法执行的结果发送会rpc的调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl;
    }
    conn->shutdown(); // 模拟http的短链接服务，由rpcprovider主动断开连接
}
