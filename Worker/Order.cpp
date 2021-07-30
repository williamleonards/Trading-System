//
// Created by William Leonard Sumendap on 7/10/20.
//

using namespace std;

#include <iostream>
#include "Order.h"

Order::Order(bool type_, int amt_, int price_, int issuerID_, int id_) {
    type = type_;
    amt = amt_;
    price = price_;
    issuerID = issuerID_;
    id = id_;
    valid = true;
}

bool Order::getType() {
    return type;
}

int Order::getID() {
    return id;
}

int Order::getIssuerID() {
    return issuerID;
}

int Order::getAmt() {
    return amt;
}

void Order::setAmt(int amt_) {
    amt = amt_;
}

int Order::getPrice() {
    return price;
}

bool Order::checkValid() {
    return valid;
}

void Order::setInvalid() {
    valid = false;
}

bool Order::equals(Order *other)
{
    if (other == nullptr) return false;
    return this->type == other->type && this->id == other->id && this->amt == other->amt &&
           this->price == other->price &&
           this->issuerID == other->issuerID && this->id == other->id;
}

void Order::print() {
    cout << "type: " << type << " amt: " << amt << " price : " << price << " issuerID: " << issuerID << " id: " << id
         << endl;
}