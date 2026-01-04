#pragma once
#include "mprpcconfig.h"
// mprpc框架的基础类的初始化
class MprpcApplication
{
public:
    static void Init(int argc, char **argv);
    static MprpcApplication& GetInstance();
    static MprpcConfig& GetConfig();
private:
    
    static MprpcConfig m_config;
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete;// 禁止拷贝
    MprpcApplication(MprpcApplication&&) = delete; // 禁止移动
};