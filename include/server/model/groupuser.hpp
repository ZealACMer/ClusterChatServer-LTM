#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

//群组用户,多了一个role角色信息，从User类直接继承，复用User的其他信息
class GroupUser : public User 
{
public:
    void setRole(std::string role) {this->role = role;}
    std::string getRole() {return this->role;}

private:
    std::string role;
};

#endif