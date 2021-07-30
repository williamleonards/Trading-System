//
// Created by William Leonard Sumendap on 7/10/20.
//

#include "User.h"

User::User(int id_, string name_)
{
    id = id_;
    name = name_;
    nextOrderID = 0;
}

User::~User() // deletes only bought trades so no trade instance get deleted twice, orders are deleted on ~TradeEngine()
{
    for (auto v : bought)
    {
        delete v;
    }
}

int User::getID()
{
    return id;
}

string User::getName()
{
    return name;
}

unordered_map<int, Order *> *User::getOrders()
{
    return &orders;
}

vector<Trade *> *User::getSold()
{
    return &sold;
}

vector<Trade *> *User::getBought()
{
    return &bought;
}

Order *User::issueOrder(bool type_, int price_, int amt_)
{
    int orderID = nextOrderID;
    nextOrderID++;
    Order *order = new Order(type_, amt_, price_, id, orderID);
    orders[orderID] = order;
    return order;
}