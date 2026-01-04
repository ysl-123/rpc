#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// 异步写日志的日志队列
template <typename T>
class LockQueue
{
public:
    void Push(const T &data)
    {   
        std::lock_guard<std::mutex> lock(m_mutex);//除了括号之后会自动将锁释放掉
        m_queue.push(data);
        m_condvariable.notify_one();//唤醒 m_condvariable.wait(lock);这个的wait状态
    }
    T Pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        //while是为了防止虚假唤醒，类似notify_all()
        while (m_queue.empty())
        {
            // 日志队列为空，线程进入wait状态
            m_condvariable.wait(lock);
        }

        T data = m_queue.front();
        m_queue.pop();
        return data;
    }

private:
    std::queue<T>  m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable;
};