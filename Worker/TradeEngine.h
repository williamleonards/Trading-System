//
// Created by William Leonard Sumendap on 7/10/20.
//

#ifndef WORKER_TRADEENGINE_H
#define WORKER_TRADEENGINE_H

using namespace std;

#include <iostream>
#include<vector>
#include<string>
#include<list>
#include<unordered_map>
#include<map>
#include<pthread.h>

#include "Order.h"
#include "Trade.h"
#include "User.h"

class TradeEngine
{
public:
    TradeEngine();

    ~TradeEngine();

    // TODO: CONVERT TO STRING USERNAME
    int createUser(string name);

    /* Place a new buy order at price `price` with amount (volume) of `amt`
     * returns a vector of Trades that occur when this order is placed.
     * A trade occurs when the incoming order wants to buy at a price higher than pending sell orders,
     * at a price specified on each of the sell order.*/
    vector<Trade *> placeBuyOrder(int issuerID, int price, int amt);

    /* Place a new sell order at price `price` with amount (volume) of `amt`
     * returns a vector of Trades that occur when this order is placed.
     * A trade occurs when the incoming order wants to sell at a price lower than pending buy orders,
     * at a price specified on each of the buy order.*/
    vector<Trade *> placeSellOrder(int issuerID, int price, int amt);

    // TODO: RETURN AN INT/BOOL REPRESENTING STATUS CODE
    // Delete an order specified by `issuerID` and `orderID`. Does nothing if parameters are invalid.
    void deleteOrder(int issuerID, int orderID);

    /* Get the volume of orders on the buy tree at all price points (in descending order).
     * Returns a vector of (price point, volume of orders at that price). */
    vector<pair<int, int>> getPendingBuys();

    /* Get the volume of orders on the sell tree at all price points (in ascending order).
     * Returns a vector of (price point, volume of orders at that price). */
    vector<pair<int, int>> getPendingSells();

    // Returns a vector of orders that are pending from the user with id `userID`
    vector<Order *> getPendingOrders(int userID);

    // Gets the buy history of the user with id `userID`. Returns a vector of trades involving the user as the buyer.
    vector<Trade *> *getBuyTrades(int userID);

    // Gets the sell history of the user with id `userID`. Returns a vector of trades involving the user as the seller.
    vector<Trade *> *getSellTrades(int userID);

private:
    pthread_mutex_t usersLock;
    pthread_mutex_t buyLock;
    pthread_mutex_t sellLock;
    unordered_map<int, User *> users;
    int nextUserID;

    // Ordered map from a price point to a pair of (volume of sell orders, list of sell orders) at that price
    map<int, pair<int, list<Order *> *> *> sellTree;

    // Ordered map from a price point to a pair of (volume of buy orders, list of buy orders) at that price
    map<int, pair<int, list<Order *> *> *> buyTree;

    // helper methods

    // Check if the first order in `lst` is stale (i.e., deleted). If so, remove it from `lst`.
    bool firstOrderIsStale(list<Order *> *lst);

    // Put the remaining order with the specified parameters to the corresponding tree.
    void putRemainingOrderOnTree(bool buyOrSell, User *user, int price, int remaining);

    // Generate trades that occur when an incoming trade is placed and update its remaining quantity accordingly.
    vector<Trade *> generateTrades(bool buyOrSell, int &price, int &issuerID, int &remaining);

    /* Consume pending orders at price point `currPrice` and put it into `ans`. Updates the volume at this price point
     * (`amtLeft`) and the remaining quantity in the incoming order (`remaining`) accordingly.
     * Also updates buyers' and sellers' accounts whenever a trade occurs. */
    void consumePendingOrders(bool buyOrSell, int &issuerID, int &remaining, int &amtLeft, int &currPrice,
                              list<Order *> *orders, vector<Trade *> &ans);

    // TODO: Make friend method for testing purposes
    /* Gets the total volume in the entire system: sum of volumes in buy and sell tree,
     * as well as the total volume of trades (this is double counted). Excludes deleted volumes.
     * Only used for concurrency tests. */
    long long getTotalVolume();
};

#endif //WORKER_TRADEENGINE_H
