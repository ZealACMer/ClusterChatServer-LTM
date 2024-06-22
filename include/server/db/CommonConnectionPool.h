#ifndef COMMONCONNECTIONPOOL_H
#define COMMONCONNECTIONPOOL_H
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread> //绑定器、生产者
#include <condition_variable>
#include <memory>
#include <functional>
#include "Connection.h"
/*
    实现连接池功能模块
*/

//采用线程安全的单例模式
class ConnectionPool
{
public:
    //获取连接池对象实例
    static ConnectionPool* getConnectionPool();
    //给外部提供接口，从连接池获取一个可用的空闲连接
    //利用智能指针自动释放资源
    std::shared_ptr<Connection> getConnection();
private:
    //单例#1 构造函数私有化
    ConnectionPool(); 
    //从配置文件中加载配置项
    bool loadConfigFile();

    //运行在独立的线程中，专门负责生产新连接
    void produceConnectionTask(); //写成成员函数，方便访问成员变量

    //扫描空闲时间超过maxIdleTime的连接，进行对应的连接回收
    void scannerConnectionTask();

    std::string _ip; //mysql的ip地址
    unsigned short _port; //mysql的端口号3306
    std::string _username; //mysql登陆用户名
    std::string _password; //mysql登陆密码
    std::string _dbname; //连接的数据库名称
    int _initSize; //连接池的初始连接量
    int _maxSize; //连接池的最大连接量
    int _maxIdleTime; //连接池的最大空闲时间
    int _connectionTimeout; //连接池获取连接的超时时间


    std::queue<Connection*> _connectionQue; //存储mysql连接的队列
    std::mutex _queueMutex; //因为连接队列是多线程访问，所以提供维护连接队列线程安全的互斥锁
    std::atomic_int _connectionCnt; //记录连接所创建的connection连接的总数量，此变量是多线程安全的
    std::condition_variable cv; //设置条件变量，用于连接生产线程和消费线程的通信
};
#endif