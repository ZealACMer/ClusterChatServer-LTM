# ClusterChatServer-LTM
基于Lightweighted Tcp Muduo(LTM)的C++集群聊天服务器   「Aliyun CentOS 7.9 64位」

## 项目背景
构建一个 C++ 集群聊天服务器项目是一个旨在提供高效、可扩展、高可用性的即时通信系统。这样的项目通常被设计来支持跨多个服务器节点的大规模并发连接和消息传递，使其特别适用于需要处理大量实时消息的应用场景，如社交网络、在线游戏、企业通信平台等。

本项目主要实现的功能如下所示：
* 使用轻量级的LTMuduo网络库提供高并发的网络I/O服务，使用函数对象和事件回调机制，将网络模块和业务模块进行解耦，有利于业务模块的升级和改进。
* 使用Json作为通信协议，对网络消息进行序列和反序列化。
* 运用Nginx的TCP负载均衡机制，实现服务器集群功能，提升后端服务的并发处理能力。
* 应用Redis的发布/订阅功能，实现集群中的跨服务器消息通信。
* 使用MySQL作为数据库，采用读写分离、主从复制的模式提高并发性及可用性。

## ClusterChatServer-LTM集群聊天服务器框架
<div align="middle">
<img width="700" height="590" alt="mrpc框架图" src="https://github.com/ZealACMer/ClusterChatServer-LTM/assets/16794553/8d5b5587-0304-44d5-9ae6-a40bc2bf13d6">
</div>
                        
## 环境配置
#### Nginx的安装(本项目使用v1.22.1)
请参照[https://nginx.org/](https://nginx.org/)

#### LTMuduo网络库的安装
请参照[https://github.com/ZealACMer/LTMuduo](https://github.com/ZealACMer/LTMuduo)

#### Redis的安装(本项目使用v7.0.11)
请参照[https://redis.io/](https://redis.io/)

#### Hiredis的安装(本项目使用v1.2.0)
请参照[https://github.com/redis/hiredis/](https://github.com/redis/hiredis/)

#### MySQL的安装(本项目使用v8.1.0)
请参照[https://www.mysql.com/downloads/](https://www.mysql.com/downloads/)

#### Mycat的安装(本项目使用v1.6.7)
请参照[https://github.com/MyCATApache/Mycat-Server](https://github.com/MyCATApache/Mycat-Server)

## 构建
本项目提供了一键构建脚本autobuild.sh，可一键进行编译及构建。（本项目使用的建表SQL在ClusterChatServer.sql中，需按照Mycat的说明文档，配置好逻辑库和物理库的映射关系）

## 使用示例
- 首先，分别开启两个ternimal，分别输入./ChatServer 127.0.0.1 6000 ./ChatServer 127.0.0.1 6002，启动两个服务器程序。
- 然后，分别启动mysql服务器(服务端口3306)，Nginx服务器(服务端口8000)，及Mycat服务器(服务端口8066)。
- 接着，分别开启两个ternimal，分别输入./ChatClient 127.0.0.1 8000，连接到Nginx服务。
- 最后，根据命令提示，进行注册、登录、聊天等相关业务。
