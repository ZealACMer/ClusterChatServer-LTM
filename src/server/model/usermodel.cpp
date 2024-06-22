#include "model/usermodel.hpp"
#include "db/Connection.h"
#include "db/CommonConnectionPool.h"
#include <iostream>

// User表的增加方法
bool UserModel::insert(User &user)
{
    // #1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values ('%s', '%s', '%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str()); //user.getName()得到是string类型，转为字符串char*类型 -> .c_str()

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        if(sp->update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(sp->getConnection()));
            return true;
        }
    }

    return false;
}


// 根据用户id查询用户信息
User UserModel::query(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        MYSQL_RES* res = sp->query(sql); // 如果查询到，返回值应该为一行数据。因为这是返回值为指针，所以系统内部动态开辟了相应的内存资源
        if(res != nullptr)
        {
           MYSQL_ROW row =  mysql_fetch_row(res); // typedef char **MYSQL_ROW;		/* return data as array of strings */
           if(row != nullptr)
           {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res); //释放系统内部动态开辟的内存资源，否则内存资源会不断地泄漏
                return user;
           }
        }
    }  

    return User(); //根据User()的默认参数值，判断是否出错,如id == -1，不过这里的逻辑不严谨，需要排除注册时有人把id设置为-1
}


//更新用户的状态信息
bool UserModel::updateState(User user)
{
    // #1 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",
        user.getState().c_str(), user.getId()); //user.getName()得到是string类型，转为字符串char*类型 -> .c_str()

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        if(sp->update(sql))
        {
            return true;
        }
    }

    return false;
}

//重置用户的状态信息
void UserModel::resetState()
{
    //组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    std::shared_ptr<Connection> sp = cp->getConnection();
    if(sp != nullptr)
    {
        sp->update(sql);
    }
}