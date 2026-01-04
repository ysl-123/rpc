#include "mprpcconfig.h"
#include <iostream>
#include <string>
// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    //实际上是./provider -i test.conf，所以加载的文件实际上是和可执行文件在同级目录下，所以可以直接加载
    FILE *pf = fopen(config_file, "r");
    if (nullptr == pf)
    {
        std::cout << config_file << " is note exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    // 读取所谓的test.config文件时。1.注释就是#  2.正确的就是类似zookeeperip=127.0.0.1 3.去掉开头就是正确的前面有个空格
    //feof 是 C 标准库函数，用来 判断文件指针是否到达文件末尾（End Of File）
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        // 去掉字符串前面多余的空格
        std::string src_buf(buf);
        Trim(src_buf);
        // 判断#的注释和 假如一整行都是空
        if (src_buf[0] == '#' || src_buf.empty())
        {
            continue;
        }
        int idx;
        // 解析配置项
        idx = src_buf.find('=');
        if (idx == -1)
        {
            // 配置项不合法
            continue;
        }

        std::string key;
        std::string value;
        key = src_buf.substr(0, idx);
        Trim(key);
        //rpcserverip=127.0.01\n
        int endidx=src_buf.find('\n',idx);
        value = src_buf.substr(idx + 1, endidx - idx-1);
        Trim(value);
        m_configMap.insert({key, value});
    }
}

// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if (it == m_configMap.end())
    {
        return "";
    }
    return it->second;
}

void MprpcConfig::Trim(std::string &src_buf){
 
        int idx = src_buf.find_first_not_of(' ');
        if (idx != -1)
        {
            // 说明字符串前面有空格
            src_buf = src_buf.substr(idx, src_buf.size() - idx);
        }
        // 去掉字符串后面多余的空格
        idx = src_buf.find_last_not_of(' ');
        if (idx != -1)
        {
            // 说明字符串后面有空格
            src_buf = src_buf.substr(0, idx + 1);
        }
}