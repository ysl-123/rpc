#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include "mprpccontroller.h"
#include "logger.h"
class FriendService : public fixbug::FriendServiceRpc
{
public:
//这里的userid实际上就是某个人的id，找的就是这个id对应的朋友的id
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout << "do GetFriendsList service!,userid"<<userid<<std::endl;
        std::vector<std::string> vec;
        vec.push_back("gao yang");
        vec.push_back("liu hong");
        vec.push_back("wang shuo");
        return vec;
    }
    // 重写基类方法
    void GetFriendsList(::google::protobuf::RpcController *controller,
                        const ::fixbug::GetFriendsListRequest *request,
                        ::fixbug::GetFriendsListResponse *response,
                        ::google::protobuf::Closure *done)
    {
        uint32_t userid = request->userid();
        std::vector<std::string> friendsList = GetFriendsList(userid);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (std::string &name : friendsList)
        {
            //新增一个空的字符串元素，返回它的指针，实际就是为了后面response给客户端的时候有返回值而已
            std::string *p = response->add_friends();//你想response里本来就有所谓的friends列表所以就是要添加呀
            *p = name;
        }
        done->Run();
    }
};

int main(int argc, char **argv)
{   
    LOG_INFO("firstmessage");
    LOG_ERROR("first serror");
    // 调用框架的初始化操作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());
 
    // 启动一个rpc服务发布节点  Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}
