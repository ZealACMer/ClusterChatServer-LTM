#ifndef GROUP_H 
#define GROUP_H

#include <vector>
#include <string>

#include "groupuser.hpp"

class Group  //多对多的表关系，中间需要插入一张中间表
{
public:
    Group(int id = -1, std::string name = "", std::tring desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id) {this->id = id;}
    void setName(std::string name) {this->name = name;}
    void setDesc(std::string desc) {this->desc = desc;}

    int getId() {return this->id;}
    std::string getName() { return this->name;}
    std::string getDesc() { return this->desc;}
    std::vector<GroupUser> &getUsers() {return this->users;}

private:
    int id; //群组id
    std::string name; //群组名称
    std::string desc; //群组描述
    std::vector<GroupUser> users;
};

#endif