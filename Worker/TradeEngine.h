//
// Created by William Leonard Sumendap on 7/10/20.
//

#ifndef WORKER_TRADEENGINE_H
#define WORKER_TRADEENGINE_H

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <map>
#include <pthread.h>

#include <pqxx/pqxx>

#include <nlohmann/json.hpp>

using namespace std;

using json = nlohmann::json;

class TradeEngine
{
public:
    TradeEngine(string conn);

    // Register a new user with the specified password
    json createUser(string name, string password);

    // Verifies if the user password is correct for login purposes
    json loginUser(string username, string password);

    /* Place a new buy order at price `price` with amount (volume) of `amt`
        * returns a vector of Trades that occur when this order is placed.
        * A trade occurs when the incoming order wants to buy at a price higher than pending sell orders,
        * at a price specified on each of the sell order.*/
    json placeBuyOrder(string buyer, int price, int amt, string ticker);

    /* Place a new sell order at price `price` with amount (volume) of `amt`
        * returns a vector of Trades that occur when this order is placed.
        * A trade occurs when the incoming order wants to sell at a price lower than pending buy orders,
        * at a price specified on each of the buy order.*/
    json placeSellOrder(string seller, int price, int amt, string ticker);

    // Delete an order specified by `issuerID` and `orderID`. Does nothing if parameters are invalid.
    json deleteBuyOrder(string username, long long orderID);

    // Delete an order specified by `issuerID` and `orderID`. Does nothing if parameters are invalid.
    json deleteSellOrder(string username, long long orderID);

    /* Get the volume of orders on the buy tree at all price points (in descending order).
        * Returns a vector of (price point, volume of orders at that price). */
    json getBuyVolumes(string ticker);

    /* Get the volume of orders on the sell tree at all price points (in ascending order).
        * Returns a vector of (price point, volume of orders at that price). */
    json getSellVolumes(string ticker);

    // Returns a vector of orders that are pending from the user with id `userID`
    json getPendingBuyOrders(string username);

    // Returns a vector of orders that are pending from the user with id `userID`
    json getPendingSellOrders(string username);

    // Gets the buy history of the user with id `userID`. Returns a vector of trades involving the user as the buyer.
    json getBuyTrades(string username);

    // Gets the sell history of the user with id `userID`. Returns a vector of trades involving the user as the seller.
    json getSellTrades(string username);

private:
    void prepareStatements();
    string hashPassword(const string& password);
    
private:
    pqxx::connection C;
};

#endif //WORKER_TRADEENGINE_H