#ifndef CONNECTION_H
#define CONNECTION_H
#include <mysql.h>
#include <string>
#include <ctime>

/*
    实现MySQL数据库的增删改查操作
*/

class Connection
{
public:
    //初始化数据库连接
    Connection();
    //释放数据库连接资源
    ~Connection();
    //连接数据库
    bool connect(std::string ip, unsigned short port, std::string username, std::string password, std::string dbname);
    //更新操作 insert delete update
    bool update(std::string sql);
    //查询操作
    MYSQL_RES* query(std::string sql);
    //获取链接
	MYSQL* getConnection();
    //刷新一下连接的起始空闲时间点
    void refreshAliveTime() {_alivetime = clock();}
    //返回存活的时间
    clock_t getAliveTime() const {return clock() - _alivetime;}

private:
    MYSQL *_conn; //表示和MySQL Server的一条连接
    clock_t _alivetime; //记录进入空闲状态后的起始时间
};
#endif