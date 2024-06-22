#include "CommonConnectionPool.h"
#include <iostream>
#include <cstdio>
#include <chrono>


//线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
    //因为是static对象，所以执行到该代码时，只初始化一次
    static ConnectionPool pool; //对于静态局部变量的初始化，由编译器自动进行lock和unlock
    return &pool;
}

bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("mysql.cnf", "r");
    if(pf == nullptr)
    {
        std::cout << __FILE__ << ":" << __LINE__ << " " << __TIMESTAMP__ << " : " << "mysql.cnf file is not exist!" << std::endl;
        return false;
    }

    while(!feof(pf))
    {
        char line[1024] = {0};
        fgets(line, 1024, pf); //一次读取一行
        std::string str = line;
        int idx = str.find('=', 0);
        if(idx == -1) //无效分配置项
        {
            continue;
        }

        int endidx = str.find('\n', idx);
        std::string key = str.substr(0, idx);
        std::string value = str.substr(idx + 1, endidx - idx - 1);

        if(key == "ip")
        {
            _ip = value;
        }
        else if(key == "port")
        {
            _port = atoi(value.c_str());
        }
        else if(key == "username")
        {
            _username = value;
        }
        else if(key == "password")
        {
            _password = value;
        }
        else if(key == "dbname")
        {
            _dbname = value;
        }
        else if(key == "initSize")
        {
            _initSize = atoi(value.c_str());
        }
        else if(key == "maxSize")
        {
            _maxSize = atoi(value.c_str());
        }
        else if(key == "maxIdleTime")
        {
            _maxIdleTime = atoi(value.c_str());
        }
        else if(key == "connectionTimeout")
        {
            _connectionTimeout = atoi(value.c_str());
        }
    }
    return true;
}

//连接池的构造
ConnectionPool::ConnectionPool()
{
    //是否加载配置项
    if(!loadConfigFile())
    {
        return;
    }
    
    //创建初始数量的连接，此时还没有线程争用连接的问题，所以不用做互斥操作
    for(int i = 0; i < _initSize; ++i)
    {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime(); //刷新一下连接空闲的起始时间
        _connectionQue.push(p);
        _connectionCnt++;
    }

    //启动一个新的线程，作为连接生产者 linux thread => pthread_create
    //thread produce(std::bind(&ConnectionPool::produceConnectionTask, this)); //this表示给成员方法绑定当前对象，不然成员函数不能作为线程函数，线程函数都是C接口

    thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();

    //启动一个新的定时线程，扫描空闲时间超过maxIdleTime的连接，进行对应的连接回收
    thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach(); //设置分离成守护线程，当主线程结束时，这些守护线程自动结束


}

//运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
    for(;;)
    {
        std::unique_lock<mutex> lock(_queueMutex); //生产者在这里拿到锁，消费者就拿不到该锁了
        while(!_connectionQue.empty())
        {
            cv.wait(lock); //在条件变量上等待队列变空;队列不空，此处生产线程进入等待状态,此时锁会得到释放，消费者线程可以进入队列取得连接
        }

        //连接数量没有到达上限，继续创建新的连接
        if(_connectionCnt < _maxSize)
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime(); //刷新一下连接空闲的起始时间
            _connectionQue.push(p);
            _connectionCnt++;
        }

        //通知消费者线程，可以消费连接了
        cv.notify_all();
    }//队列为空的话，执行到此处，释放锁

}
    
//给外部提供接口，从连接池中获取一个可用的空闲连接
std::shared_ptr<Connection> ConnectionPool::getConnection()
{
    std::unique_lock<mutex> lock(_queueMutex);
    while(_connectionQue.empty())
    {
        //与sleep不同，不是睡眠一定的时间，而是等待_connectionTimeout时间
        //超时醒来
        if(cv_status::timeout == cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout))) 
        {
            if(_connectionQue.empty())
            {
                std::cout << __FILE__ << ":" << __LINE__ << " " << __TIMESTAMP__ << " : " << "获取空闲连接超时了...获取连接失败!" << std::endl;
                return nullptr;
            }
        }
        //有新的连接可以使用，则跳出循环
    }

    /*
    shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于调用connection的析构函数，connection
    就被close掉了，这里需要自定义shared_ptr释放资源的方式，把connection直接归还到queue当中
    */
    std::shared_ptr<Connection> sp(_connectionQue.front(), 
        [&](Connection *pcon){ //&表示以引用的方式获取外部的所有变量
            //这里是在服务器应用线程中调用的，一定要考虑队列的线程安全，所以加锁
            std::unique_lock<mutex> lock(_queueMutex);
            pcon->refreshAliveTime(); //刷新一下连接空闲的起始时间
            _connectionQue.push(pcon);
        }); //出了右括号作用域，锁自动释放
    _connectionQue.pop();
    //消费完连接之后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接
    cv.notify_all();
    return sp;
}

//扫描空闲时间超过maxIdleTime的连接，进行对应的连接回收
void ConnectionPool::scannerConnectionTask()
{
    for(;;)
    {
        //通过sleep模拟定时效果
        this_thread::sleep_for(std::chrono::seconds(_maxIdleTime)); //秒
        
        //扫描整个队列，释放多余的连接
        std::unique_lock<mutex> lock(_queueMutex); //因为要对队列进行操作，所以事先加锁
        while(_connectionCnt > _initSize)
        {
            Connection *p = _connectionQue.front();
            if((double)p->getAliveTime()/ CLOCKS_PER_SEC >= _maxIdleTime)
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p; //调用~Connection()释放连接
            }
            else
            {
                break; //队头的连接空闲时间最久，如果队头连接的空闲时间都没有超过_maxIdleTime的话，直接break
            }

        }
    }
}