#include "logger.h"
#include <time.h>
#include <iostream>

// 获取日志的单例
Logger &Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    // 启动专门的写日志线程
    // 其实就是类似于所谓的thread writeLogTask(func)
    std::thread writeLogTask([&]()
                             {
        for (;;)
        {
            // 获取当前的日期，然后取日志信息，写入相应的日志文件当中 a+
           // now 的值大概是从 1970-01-01 00:00:00 UTC 到 2025-11-08 20:30:45 的秒数，比如：1760347845
           time_t now = time(nullptr);
           //将其表示为多少年多少月多少日秒等
           tm *nowtm = localtime(&now);

           char file_name[128];
           //所以一天只会有一个文件，而且名字就是第一次创建的时间
           sprintf(file_name, "%d-%d-%d-log.txt", nowtm->tm_year+1900, nowtm->tm_mon+1, nowtm->tm_mday);

           FILE *pf = fopen(file_name, "a+");
           if (pf == nullptr)
           {
           std::cout << "logger file : " << file_name << " open error!" << std::endl;
             exit(EXIT_FAILURE);
           }

          std::string msg = m_lckQue.Pop();

          char time_buf[128] = {0};
          sprintf(time_buf, "%d:%d:%d =>[%s]", nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec,(m_loglevel == INFO ? "info" : "error"));
          msg.insert(0, time_buf);
          msg.append("\n");
          //把 time_buf 这个字符串插到 msg 最前面（位置 0）
          fputs(msg.c_str(), pf);
          fclose(pf);
        } });
    // 设置分离线程，守护线程
    writeLogTask.detach();
}

// 设置日志级别
void Logger::SetLogLevel(LogLevel level)
{
    m_loglevel = level;
}

// 写日志，把日志信息写入lockqueue缓冲区当中
void Logger::Log(std::string msg)
{
    m_lckQue.Push(msg);
}
