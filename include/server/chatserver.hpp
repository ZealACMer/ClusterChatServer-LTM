#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <ltmuduo/TcpServer.h>

//聊天服务器的主类
class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg);
    // 启动服务
    void start();
private:
    //连接事件的回调函数
    void onConnection(const TcpConnectionPtr&);
    //读写事件的回调函数
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);
    TcpServer _server; //组合的ltmuduo库实现服务器功能的类对象
    EventLoop *_loop; //指向事件循环对象的指针
};

#endif