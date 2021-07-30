//
// Created by William Leonard Sumendap on 7/10/20.
//

#include "Trade.h"

Trade::Trade(int amt_, int price_, int buyerID_, int sellerID_)
{
    amt = amt_;
    price = price_;
    buyerID = buyerID_;
    sellerID = sellerID_;
}

int Trade::getAmt()
{
    return amt;
}

int Trade::getPrice()
{
    return price;
}

int Trade::getBuyerID()
{
    return buyerID;
}

int Trade::getSellerID()
{
    return sellerID;
}

bool Trade::equals(Trade *other)
{
    if (other == nullptr) return false;
    return this->amt == other->amt && this->price == other->price && this->buyerID == other->buyerID
           && this->sellerID == other->sellerID;
}