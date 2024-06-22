#include "model/friendmodel.hpp"
#include "db/Connection.h"
#include "db/CommonConnectionPool.h"

//添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    //组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        sp->update(sql);
    }
}

//返回用户好友列表
std::vector<User> FriendModel::query(int userid)
{
    // 组装sql语句
    //friend表的联合主键(userid, friendid)是为了互为好友的两人，只插入一条信息，避免信息冗余
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d union select a.id, a.name, a.state from user a inner join friend b on b.userid = a.id where b.friendid = %d", userid, userid);

    std::vector<User> vec;

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        MYSQL_RES* res = sp->query(sql);
        if(res != nullptr)
        {
            //把userId用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }

            mysql_free_result(res);
            return vec;            
        }        
    }
    return vec;    
}