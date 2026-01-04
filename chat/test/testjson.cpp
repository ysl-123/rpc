#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

// json序列化示例1
string func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    // dump() 会把 JSON 对象   序列化  标准 JSON 格式的字符串
    // 返回类型是 std::string
    string sendBuf = js.dump();

    // sendBuf.c_str() 返回 const char*，cout 可以打印
    cout << sendBuf.c_str() << endl;

    // 方法2：直接打印 JSON 对象
    // nlohmann::json 重载了 operator<<
    // 内部实现其实也是调用 dump()
    // cout << js << endl;
    return  sendBuf;
}

void func2()
{
    json js;
    // 添加数组
    js["id"] = {1, 2, 3, 4, 5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    cout << js << endl;
}

// json序列化示例代码3
void func3()
{
    json js;

    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);

    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});

    js["path"] = m;

    string sendBuf = js.dump(); // json数据对象 => 序列化 json字符串
    cout << sendBuf << endl;
}
int main()
{
    func1();

    string recvBuf = func1();
    // 数据的反序列化  json字符串 => 反序列化 json数据对象（看作容器，方便访问）
    json jsbuf = json::parse(recvBuf);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;

    return 0;
}