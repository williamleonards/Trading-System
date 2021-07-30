//
// Created by William Leonard Sumendap on 7/10/20.
//

#ifndef WORKER_ORDER_H
#define WORKER_ORDER_H

using namespace std;

#include<vector>
#include<string>
#include<list>
#include<unordered_map>
#include<map>

class User;

class Order {
public:
    Order(bool type_, int amt_, int price_, int issuerID_, int id_);

    bool getType();

    int getID();

    int getIssuerID();

    int getAmt();

    void setAmt(int amt_);

    int getPrice();

    // Checks if the order is stale (i.e., has been deleted by the deleteOrder command).
    bool checkValid();

    // Set the order to be stale.
    void setInvalid();

    // Logical equals: return true if other != NULL and all the corresponding fields are equal
    bool equals(Order *other);

    void print();

private:
    bool type; // true = buy, false = sell
    int id;
    int issuerID;
    int amt;
    int price;
    bool valid;
};

#endif //WORKER_ORDER_H
