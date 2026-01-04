#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <semaphore.h>
#include <iostream>

// 全局 watcher 回调函数
// 当 ZooKeeper 连接状态或节点变化时，这个函数会被自动调用
void global_watcher(zhandle_t *zh, int type,
                    int state, const char *path, void *watcherCtx)
{
    // 判断事件类型是不是会话事件（Session Event）
    if (type == ZOO_SESSION_EVENT)
    {
        // 判断当前会话状态是否已经连接成功
        if (state == ZOO_CONNECTED_STATE)
        {
            // 从 ZooKeeper handle 中获取我们之前绑定的上下文，这里是信号量
            sem_t *sem = (sem_t*)zoo_get_context(zh);

            // 信号量加一，表示连接已经成功，可以让等待的线程继续执行
            sem_post(sem);
        }
    }
}


ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源 MySQL_Conn
    }
}

// 连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    /*
    zookeeper_mt: 多线程版本
    zookeeper的API客户端程序提供了三个线程
    API调用线程  zoo_create, zoo_get, zoo_se
    网络I/O线程 pthread_create poll  负责与 ZooKeeper 服务端的网络通信（TCP socket）
    watcher回调线程
    */
   /* (connstr.c_str(), global_watcher, 
   会话超时时间, 这些时间内保持心跳
   客户端id,用于“断线重连恢复会话，第一次传空，恢复上一次就传入clientid_t 结构体（从上次会话中获得
    用户上下文, 
    标志位);
   */ 
   if (nullptr == m_zhandle)
   //session会话timeout时间为30秒， zkclient网络i/o线程每隔1/3的timeout时间ping一下作为心跳，防止以为会话过期，删除临时节点 
   //调用 zookeeper_init：启动 ZK 客户端三个线程 
   m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (nullptr == m_zhandle)//防止初始化失败，
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);//只是将这个锁挂在句柄上，等句柄观察到和服务器建立连接之后，解锁，

    sem_wait(&sem);
    std::cout << " ZooKeeper connected successfully" << std::endl;
}
//state就是看临时还是永久
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (ZNONODE == flag) // 表示path的znode节点不存在
    {
        // 创建指定path的znode节点了
        flag = zoo_create(m_zhandle, path, data, datalen,
                         &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (flag == ZOK)
        {
            std::cout << "znode create success... path:" << path << std::endl;
        }
        else
        {
            std::cout << "flag:" << flag << std::endl;
            std::cout << "znode create error... path:" << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK)
    {
        std::cout << "get znode error... path:" << path << std::endl;
        return "";
    }
    else
    {
        return buffer;
    }
}