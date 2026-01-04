#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

// 其实将userservice拿出来，不与main里面的执行混淆会更加清晰理解到main只是调用他们几个
/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
*/
class UserService : public fixbug::UserServiceRpc // 使用在rpc服务端（rpc服务的提供者）
{
public:
    //实际上所谓的protobuf已经在创建了所谓的login方法，但是具体的实现是没有的，所以需要我们自己写具体的实现
    bool Login(std::string name, std::string pwd) // 这是重载不是重写虚函数
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service:Register" << std::endl;
        std::cout << "id:" << id << "name:" << name << " pwd:" << pwd << std::endl;
        return true;
    }
    /*
    重写基类UserServiceRpc的虚函数 下面这些方法都是框架直接调用的
    1. caller ===> 发起Login(LoginRequest) => 通过muduo => 到达callee，此时还是框架
    2. callee ===> 框架自动将远端的Login(LoginRequest) => 交到下面重写的这个Login方法上了
    */
    // 重写
    void Login(::google::protobuf::RpcController *controller,
                       const ::fixbug::LoginRequest *request,
                       ::fixbug::LoginResponse *response,
                       ::google::protobuf::Closure *done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool login_result = Login(name, pwd); // 这里参数个数不一样就可以区分login函数了

        // 写入响应
        // 因为前面void login函数后两个参数为空，只是填写了变量，就是为了我们业务做完了之后直接填进去，填进去后一切交给框架，少我们的事情
        // 当前是写的是成功的状态  把响应写入，包括错误码，错误消息，返回值
        fixbug::ResultCode *code = response->mutable_result(); // 加上这个因为作用域
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_sucess(login_result);

        // 执行回调操作 执行响应数据的序列化和网络发送（都是由框架完成的）
        // 因为响应你还是先将响应的东西序列化之后发送出去呀，
        done->Run();
    }

    //RpcController 是 RPC 框架用于控制调用流程和传递状态的关键组件，即使当前业务逻辑中未使用，它也是框架接口设计的一部分，用于支持错误处理、超时控制等高级功能。
    void Register(::google::protobuf::RpcController *controller,
                  const ::fixbug::RegisterRequest *request,
                  ::fixbug::RegisterResponse *response,
                  ::google::protobuf::Closure *done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id, name, pwd);

        fixbug::ResultCode *code = response->mutable_result(); // 加上这个因为作用域
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_sucess(ret);

        done->Run();
    }
};

int main(int argc, char **argv)
{
    // 调用框架的初始化操作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点  Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}