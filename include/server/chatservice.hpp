#ifndef CHATSERVICE_H  //防止头文件重复包含
#define CHATSERVICE_H


#include <unordered_map>
#include <functional>
#include <mutex>

#include <ltmuduo/TcpConnection.h>
#include "json.hpp"
#include "redis/redis.hpp"

#include "model/usermodel.hpp"
#include "model/offlinemessagemodel.hpp"
#include "model/friendmodel.hpp"
#include "model/groupmodel.hpp"

using json = nlohmann::json;



//表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

//聊天服务器业务类
class ChatService 
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    //服务器异常，业务重置方法
    void reset();

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, std::string);
private:
    ChatService(); //单例模式，默认构造函数私有化

    //存储消息id和其对应的业务处理方法，可以用手动设置的json对象进行测试（因为事先已经设置好，不存在动态地增加和删除业务处理方法，所以不用注意线程安全）
    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接(根据用户的上线和下线，会不断地改变，在多线程调用的时候，要注意线程安全)
    std::unordered_map<int, TcpConnectionPtr> _userConnMap;

    //定义互斥锁，保证_userConnMap的线程安全
    std::mutex _connMutex;

    //数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    //Redis操作对象
    Redis _redis;
};

#endif