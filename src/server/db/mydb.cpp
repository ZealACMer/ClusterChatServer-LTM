#include "db/mydb.h"
#include <ltmuduo/Logger.h>

// 数据库配置信息
static std::string server = "ip地址";
static std::string user = "用户名";
static std::string password = "密码";
static std::string dbname = "Mycat的逻辑数据库名";

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr) mysql_close(_conn);
}
// 连接数据库
bool MySQL::connect()
{
        MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
        password.c_str(), dbname.c_str(), 8066, nullptr, 0);
        if (p != nullptr)
        {
            //C和C++代码默认的编码字符是ASCII码，如果不设置，从MySQL上读取的内容中文显示为"?"
            mysql_query(_conn, "set names gbk"); //设置客户端的字符集
            LOG_INFO("connect mysql success!");
        }
        else
        {
            LOG_INFO("connect mysql fail!");
        }
        return p;
}
// 更新操作
bool MySQL::update(string sql)
{
        if (mysql_query(_conn, sql.c_str()))
        {
                LOG_INFO("%s:%d:%s更新失败!", __FILE__, __LINE__, sql);

                return false;
        }
        return true;
}
// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
        if (mysql_query(_conn, sql.c_str()))
        {
                LOG_INFO("%s:%d:%s查询失败!", __FILE__, __LINE__, sql);
                return nullptr;
        }
        return mysql_use_result(_conn);
}

//获取链接
MYSQL* MySQL::getConnection()
{
    return _conn;
}
