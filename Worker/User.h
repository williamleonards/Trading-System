//
// Created by William Leonard Sumendap on 7/10/20.
//

#ifndef WORKER_USER_H
#define WORKER_USER_H

using namespace std;

#include<vector>
#include<string>
#include<list>
#include<unordered_map>
#include<map>
#include "Order.h"

class Trade;

class User
{
public:
    User(int id_, string name_);

    ~User();

    int getID();

    string getName();

    unordered_map<int, Order *> *getOrders();

    vector<Trade *> *getSold();

    vector<Trade *> *getBought();

    Order *issueOrder(bool type_, int price_, int amt_);

private:
    int id;
    string name;
    unordered_map<int, Order *> orders;
    vector<Trade *> sold;
    vector<Trade *> bought;
    int nextOrderID;
};

#endif //WORKER_USER_H
