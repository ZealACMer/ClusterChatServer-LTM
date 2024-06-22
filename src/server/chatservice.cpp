#include <vector>
#include <ltmuduo/Logger.h> //打印日志

#include "chatservice.hpp"
#include "public.hpp"

//获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service; //线程安全
    return &service;
}

//注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    //连接Redis服务器
    if(_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
    
}

//服务器异常，业务重置方法
void ChatService::reset()
{
    //把online状态的用户，设置成offline
    _userModel.resetState();
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp){
            LOG_ERROR("msgid: %d cannot find handler!", msgid);
        };
    }
    else
    {
        return _msgHandlerMap[msgid]; //注意其有副作用，如果存在则返回，如果不存在，则创建数据，所以用find查找key值
    }
}

//处理登录业务 id pwd 
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    std::string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId() == id && user.getPwd() == pwd)
    {
        if(user.getState() == "online")
        {
            //该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "This account is using, input another account!";
            conn->send(response.dump());
        }
        else
        {
            //登录成功，记录用户的连接信息
            {
                std::lock_guard<mutex>  lock(_connMutex); //lock_guard类似于智能指针，构造时加锁，析构时解锁
                _userConnMap.insert({id, conn});
            }//锁的粒度要尽可能的小，出作用域lock_guard析构，自动解锁

            //id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);


            //数据库增删改查的并发操作，对应的线程安全由MySQL保证
            //登录成功，更新用户状态信息 state offline -> online
            user.setState("online");
            _userModel.updateState(user);

            //以下合成的json，是局部变量，每个线程栈都是隔离的，不会产生冲突   
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName(); //将用户名，好友列表等信息记录在本地，不然服务器的访问压力太大

            //查询该用户是否有离线消息
            std::vector<std::string> vec = _offlineMsgModel.query(id);
            if(!vec.empty()) //如果vec不为空，则在json中加入如下字段
            {
                response["offlinemsg"] = vec; //json功能非常强大，可以直接赋值vector
                //读取该用户的离线消息后，将该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            //查询该用户的好友信息并返回
            std::vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                //response["friends"] = userVec; 如此表述不可以，因为自定义类型User无法被json识别
                std::vector<std::string> vec2;
                for(User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            std::vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                std::vector<std::string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    std::vector<std::string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }            

            conn->send(response.dump());
        }

    }
    else
    {
        //该用户不存在，或者用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}
//处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if(state)
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());        
    }
}

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        std::lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updateState(user);

}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{   
    User user;
    {   //访问_userConnMap，要注意线程安全
        std::lock_guard<mutex> lock(_connMutex);      
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if(it->second == conn)
            {
                //从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }//出作用域，自动解锁

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //更新用户的状态信息
    if(user.getId() != -1) //确保为有效用户
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toId = js["toid"].get<int>();

    { //访问连接信息表，保证线程安全
        std::lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toId);
        //在同一台主机
        if(it != _userConnMap.end())
        {
            //toId在线，转发消息，服务器主动推送消息给toId用户
            it->second->send(js.dump());
            return;
        }
    }

    //查询toid是否在线
    //不在同一台主机，并且在线
    User user = _userModel.query(toId);
    if(user.getState() == "online")
    {
        _redis.publish(toId, js.dump());
        return;
    }
    
    //toId不在线，转储离线消息
    _offlineMsgModel.insert(toId, js.dump());

}


//添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid, friendid);
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    //存储新创建的群组消息
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group))
    {
        //存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    std::lock_guard<mutex> lock(_connMutex);
    for(int id : useridVec)
    {
        auto it = _userConnMap.find(id); //C++ STL的map不是线程安全的，所以进行加锁，避免操作的时候，其他人对_userConnMap的内容进行改动
        if(it != _userConnMap.end())
        {
            //转发群消息
            it->second->send(js.dump());
        }
        else
        {
            //查询toid是否在线
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                //存储离线群消息
                _offlineMsgModel.insert(id, js.dump());               
            }
        }
    }
}

//从Redis消息队列中获取订阅的消息,将消息上报给订阅端(channel id = userid)
//为同一台主机的原因是，上报消息给确定的主机(userid的用户订阅userid的通道，顺着这个通道，逆向上报给订阅者)
void ChatService::handleRedisSubscribeMessage(int userid, std::string msg)
{
    std::lock_guard<mutex> lock(_connMutex);
    //确定的此台主机，订阅用户在线，直接发送
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    //确定的此台主机，存储发送给订阅用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}