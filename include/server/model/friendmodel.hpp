#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "user.hpp"

//维护好友信息的操作接口方法
class FriendModel
{
public:
    //添加好友关系
    void insert(int userid, int friendid);

    //返回用户好友列表 -> 现实app中，一般都是将好友列表存储在本地，不再服务器重新加载好友信息，为了减轻服务器负担，提高响应速度，
    //这里为了简便，将好友信息当成离线信息，客户登录时，在服务器端推送给用户
    std::vector<User> query(int userid);
    
};

#endif