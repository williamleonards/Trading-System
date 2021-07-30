//
// Created by William Leonard Sumendap on 7/10/20.
//

#include "TradeEngine.h"
#include <pthread.h>

TradeEngine::TradeEngine()
{
    nextUserID = 0;
    pthread_mutex_init(&usersLock, NULL);
    pthread_mutex_init(&buyLock, NULL);
    pthread_mutex_init(&sellLock, NULL);
}

// delete all pending orders, all trades are deleted in ~User()
TradeEngine::~TradeEngine()
{
    for (auto p : users)
    {
        delete p.second;
    }
    for (auto p : sellTree)
    {
        list<Order *> *lst = p.second->second;
        for (Order *ord : *lst)
        {
            delete ord;
        }
        delete p.second;
    }
    for (auto p : buyTree)
    {
        list<Order *> *lst = p.second->second;
        for (Order *ord : *lst)
        {
            delete ord;
        }
        delete p.second;
    }
}

int TradeEngine::createUser(string name)
{

    pthread_mutex_lock(&usersLock);

    User *user = new User(nextUserID, name);
    users[nextUserID] = user;

    if (nextUserID == (1 << 31) - 1)
    {
        cout << "User limit reached" << endl;
        return -1;
    }

    nextUserID++;
    int ans = nextUserID - 1;

    pthread_mutex_unlock(&usersLock);

    return ans;
}

vector<Trade *> TradeEngine::placeBuyOrder(int issuerID, int price, int amt)
{

    pthread_mutex_lock(&usersLock);
    pthread_mutex_lock(&buyLock);
    pthread_mutex_lock(&sellLock);

    User *user = users[issuerID];
    vector<Trade *> ans;

    if (user == NULL)
    {
        cout << "User ID not known" << endl;
        return ans;
    }

    int remaining = amt;
    // generate trades by matching the incoming order with pending sell orders
    ans = generateTrades(true, price, issuerID, remaining);

    if (remaining > 0)
    { // put surplus amount on buy tree
        putRemainingOrderOnTree(true, user, price, remaining);
    }

    pthread_mutex_unlock(&sellLock);
    pthread_mutex_unlock(&buyLock);
    pthread_mutex_unlock(&usersLock);

    return ans;
}

vector<Trade *> TradeEngine::placeSellOrder(int issuerID, int price, int amt)
{

    pthread_mutex_lock(&usersLock);
    pthread_mutex_lock(&buyLock);
    pthread_mutex_lock(&sellLock);

    User *user = users[issuerID];
    vector<Trade *> ans;

    if (user == NULL)
    {
        cout << "User ID not known" << endl;
        return ans;
    }

    int remaining = amt;

    // generate trades by matching the incoming order with pending buy orders
    ans = generateTrades(false, price, issuerID, remaining);

    if (remaining > 0)
    { // put surplus amount on sell tree
        putRemainingOrderOnTree(false, user, price, remaining);
    }

    pthread_mutex_unlock(&sellLock);
    pthread_mutex_unlock(&buyLock);
    pthread_mutex_unlock(&usersLock);

    return ans;
}

void TradeEngine::deleteOrder(int issuerID, int orderID)
{ //lazy deletion

    pthread_mutex_lock(&usersLock);
    pthread_mutex_lock(&buyLock);
    pthread_mutex_lock(&sellLock);

    User *user = users[issuerID];

    if (user == NULL)
    {
        pthread_mutex_unlock(&sellLock);
        pthread_mutex_unlock(&buyLock);
        pthread_mutex_unlock(&usersLock);
        return;
    }

    unordered_map<int, Order *> *userOrders = (user->getOrders());

    if (userOrders->count(orderID) == 0)
    {
        pthread_mutex_unlock(&sellLock);
        pthread_mutex_unlock(&buyLock);
        pthread_mutex_unlock(&usersLock);
        return;
    }

    Order *order = userOrders->at(orderID);
    int price = order->getPrice();
    pair<int, list<Order *> *> *p = order->getType() ? buyTree[price] : sellTree[price];

    if (p == NULL)
    {
        pthread_mutex_unlock(&sellLock);
        pthread_mutex_unlock(&buyLock);
        pthread_mutex_unlock(&usersLock);
        return;
    }

    p->first -= order->getAmt();
    order->setInvalid();
    userOrders->erase(orderID);

    pthread_mutex_unlock(&sellLock);
    pthread_mutex_unlock(&buyLock);
    pthread_mutex_unlock(&usersLock);
}

vector<pair<int, int>> TradeEngine::getPendingBuys()
{

    pthread_mutex_lock(&buyLock);

    vector<pair<int, int>> v;

    for (auto itr = buyTree.rbegin(); itr != buyTree.rend(); itr++)
    {
        int price = itr->first;
        int vol = itr->second->first;
        if (vol != 0)
        {
            v.push_back(pair<int, int>(price, vol));
        }
    }

    pthread_mutex_unlock(&buyLock);

    return v;
}

vector<pair<int, int>> TradeEngine::getPendingSells()
{

    pthread_mutex_lock(&sellLock);

    vector<pair<int, int>> v;

    for (auto itr = sellTree.begin(); itr != sellTree.end(); itr++)
    {
        int price = itr->first;
        int vol = itr->second->first;
        if (vol != 0)
        {
            v.push_back(pair<int, int>(price, vol));
        }
    }

    pthread_mutex_unlock(&sellLock);

    return v;
}

vector<Order *> TradeEngine::getPendingOrders(int userID)
{

    pthread_mutex_lock(&usersLock);

    User *user = users[userID];
    vector<Order *> ans;

    if (user == NULL)
    {
        cout << "User ID not known" << endl;
        return ans;
    }

    auto orders = user->getOrders();

    for (auto itr = orders->begin(); itr != orders->end(); itr++)
    {
        ans.push_back(itr->second);
    }

    pthread_mutex_unlock(&usersLock);

    return ans;
}

vector<Trade *> *TradeEngine::getBuyTrades(int userID)
{

    pthread_mutex_lock(&usersLock);
    User *user = users[userID];
    pthread_mutex_unlock(&usersLock);

    if (user == NULL)
    {
        cout << "User ID not known" << endl;
        return nullptr;
    }

    return user->getBought();
}

vector<Trade *> *TradeEngine::getSellTrades(int userID)
{

    pthread_mutex_lock(&usersLock);
    User *user = users[userID];
    pthread_mutex_unlock(&usersLock);

    if (user == NULL)
    {
        cout << "User ID not known" << endl;
        return nullptr;
    }

    return user->getSold();
}

long long TradeEngine::getTotalVolume()
{

    long long totalVol = 0;

    vector<pair<int, int>> buyTree = getPendingBuys();
    vector<pair<int, int>> sellTree = getPendingSells();

    for (int i = 0; i < buyTree.size(); i++)
    {
        totalVol += buyTree[i].second;
    }

    for (int i = 0; i < sellTree.size(); i++)
    {
        totalVol += sellTree[i].second;
    }

    for (auto itr = users.begin(); itr != users.end(); itr++)
    {
        User *user = itr->second;
        vector<Trade *> *bought = user->getBought();
        for (int i = 0; i < bought->size(); i++)
        {
            totalVol += bought->at(i)->getAmt();
        }
        vector<Trade *> *sold = user->getSold();
        for (int i = 0; i < sold->size(); i++)
        {
            totalVol += sold->at(i)->getAmt();
        }
    }

    return totalVol;
}

// helper methods defined below
bool TradeEngine::firstOrderIsStale(list<Order *> *lst)
{

    Order *first = lst->front();

    if (!first->checkValid())
    {
        lst->pop_front();
        delete first;
        return true;
    }

    return false;
}

void TradeEngine::putRemainingOrderOnTree(bool buyOrSell, User *user, int price, int remaining)
{

    map<int, pair<int, list<Order *> *> *> &tree = buyOrSell ? buyTree : sellTree;
    pair<int, list<Order *> *> *p = tree[price];

    Order *leftover = user->issueOrder(buyOrSell, price, remaining);

    if (p != NULL)
    {
        p->first += remaining;
        p->second->push_back(leftover);
    }
    else
    {
        list<Order *> *lst = new list<Order *>();
        lst->push_back(leftover);
        tree[price] = new pair<int, list<Order *> *>(remaining, lst);
    }
}

vector<Trade *> TradeEngine::generateTrades(bool buyOrSell, int &price, int &issuerID, int &remaining)
{

    vector<Trade *> ans;

    // generate trades when appropriate
    if (buyOrSell)
    {

        for (auto itr = sellTree.begin(); itr != sellTree.end(); itr++)
        {

            int currPrice = itr->first;
            if (currPrice > price || remaining <= 0)
                break;

            int amtLeft = itr->second->first;
            list<Order *> *orders = itr->second->second;

            // consume pending sell orders at this price point
            consumePendingOrders(true, issuerID, remaining, amtLeft, currPrice, orders, ans);

            itr->second->first = amtLeft;
        }
    }
    else
    {

        for (auto itr = buyTree.rbegin(); itr != buyTree.rend(); itr++)
        {

            int currPrice = itr->first;
            if (currPrice < price || remaining <= 0)
                break;

            int amtLeft = itr->second->first;
            list<Order *> *orders = itr->second->second;

            // consume pending buy orders at this price point
            consumePendingOrders(false, issuerID, remaining, amtLeft, currPrice, orders, ans);

            itr->second->first = amtLeft;
        }
    }

    return ans;
}

void TradeEngine::consumePendingOrders(bool buyOrSell, int &issuerID, int &remaining, int &amtLeft,
                                       int &currPrice, list<Order *> *orders, vector<Trade *> &ans)
{

    while (remaining > 0 && !orders->empty())
    {

        if (firstOrderIsStale(orders))
            continue; // remove if current order is stale and continue

        Order *first = orders->front();
        int currAmt = first->getAmt();
        int counterpartyID = first->getIssuerID();

        // figure out who's the buyer and the seller
        int buyerID = buyOrSell ? issuerID : counterpartyID;
        int sellerID = buyOrSell ? counterpartyID : issuerID;
        User *buyer = users[buyerID];
        User *seller = users[sellerID];

        User *counterparty = users[counterpartyID];

        if (remaining < currAmt)
        { // current order not finished

            Trade *trade = new Trade(remaining, currPrice, buyerID, sellerID);
            ans.push_back(trade);

            // update buyer's and seller's finished orders
            buyer->getBought()->push_back(trade);
            seller->getSold()->push_back(trade);

            // update first order amount
            first->setAmt(currAmt - remaining);
            amtLeft -= remaining;
            remaining = 0;
        }
        else
        { // current order finished

            Trade *trade = new Trade(currAmt, currPrice, buyerID, sellerID);
            ans.push_back(trade);

            // update buyer's and seller's finished orders
            buyer->getBought()->push_back(trade);
            seller->getSold()->push_back(trade);
            orders->pop_front();

            // update counterparty's orders
            counterparty->getOrders()->erase(first->getID());
            delete first;
            remaining -= currAmt;
            amtLeft -= currAmt;
        }
    }
}
