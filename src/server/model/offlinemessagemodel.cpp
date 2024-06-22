#include "model/offlinemessagemodel.hpp"
#include "db/Connection.h"
#include "db/CommonConnectionPool.h"

//存储用户的离线消息
void OfflineMsgModel::insert(int userId, std::string msg)
{
    // #1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values (%d, '%s')", userId, msg.c_str());

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        sp->update(sql);
    }
}

//删除用户的离线消息
void OfflineMsgModel::remove(int userId)
{
    // #1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userId);

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        sp->update(sql);
    }
}

//查询用户的离线消息
std::vector<string> OfflineMsgModel::query(int userId)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userId);

    std::vector<string> vec;

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        MYSQL_RES* res = sp->query(sql); // 如果查询到，返回值应该为一行数据。因为这是返回值为指针，所以系统内部动态开辟了相应的内存资源
        if(res != nullptr)
        {
            //把userId用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;            
        }
    }
    return vec;
}
