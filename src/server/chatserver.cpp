#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <functional> // 函数绑定器bind
#include <string>
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg)
            :_server(loop, listenAddr, nameArg)
            ,_loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    // setConnectionCallback需要一个参数，设置的成员函数有两个参数(this是隐含参数)，所以用bind进行绑定，其中std::placeholders::_1是参数占位符，表示有一个参数需要传入

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    //设置线程数量
    _server.setThreadNum(4);

}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

//上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    //客户端断开链接
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown(); //释放socket资源
    }
}
//上报读写事件相关信息的回调函数，设置了4个线程，其中有3个worker线程，所以说会有3个worker线程同时访问该函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                        Buffer *buffer,
                        Timestamp time)
{
    std::string buf = buffer->retrieveAllAsString();
    //数据的反序列化
    json js = json::parse(buf);
    /*
    模块进行解耦的两种方式：第一种是接口(C++中没有接口的概念，指的是抽象类)，还有一种是使用回调
    */
   //达到的目的：完全解耦网络模块的代码和业务模块的代码
   //通过js["msgid"].get<int>()获取业务handler
   auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); //.get<int>()的作用是将json类型的数据转换为整型数据
   //回调消息绑定好的事件处理器，来执行相应的业务处理，通过回调的方法，没有暴露任何业务模块的代码，实现了解耦，以后修改业务模块代码，此处无需任何改动
   msgHandler(conn, js, time);
}