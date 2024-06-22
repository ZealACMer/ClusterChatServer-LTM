#ifndef USER_H
#define USER_H

#include <string>

//User表的ORM(对象关系映射)类
class User
{
public:
    User(int id = -1, std::string name = "", std::string pwd = "", std::string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }

    void setId(int id) {this->id = id;} //为了解耦业务代码和数据库代码
    void setName(std::string name) {this->name = name;}
    void setPwd(std::string pwd) {this->password = pwd;}
    void setState(std::string state) {this->state = state;}

    int getId() {return this->id;}
    std::string getName() {return this->name;}
    std::string getPwd() {return this->password;}
    std::string getState() {return this->state;}

private:
    int id;
    std::string name;
    std::string password;
    std::string state;
};

#endif