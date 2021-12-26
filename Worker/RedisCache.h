//
// Created by William Leonard Sumendap on 12/25/21.
//

#ifndef WORKER_REDIS_CACHE_H
#define WORKER_REDIS_CACHE_H

#include <sw/redis++/redis++.h>
#include "Poco/Net/HTTPCookie.h"
#include <nlohmann/json.hpp>

using namespace sw::redis;
using namespace Poco::Net;
using json = nlohmann::json;

class RedisCache
{
public:
    RedisCache(json config);
    // pending buy/sell operations
    bool readPendingBuySell(const std::string &username, bool isBuy,
        json &response);
    bool newPendingBuySell(const std::string &username, bool isBuy,
        json &entry);
    bool insertPendingBuySell(const std::string &username, bool isBuy,
        json &order);
    bool removePendingBuySell(const std::string &username, bool isBuy,
        int orderId);
    // view buy/sell tree operations
    bool readBuySellTree(const std::string &ticker, bool isBuy,
        json &response);
    bool newBuySellTree(const std::string &ticker, bool isBuy,
        json &entry);
    bool insertBuySellTree(const std::string &ticker, bool isBuy,
        int price, int amt);
    bool removeBuySellTree(const std::string &ticker, bool isBuy,
        int price, int amt);
    // buy/sell history operations
    bool readBuySellHistory(const std::string &username, bool isBuy,
        json &response);
    bool newBuySellHistory(const std::string &username, bool isBuy,
        json &entry);
    bool insertBuySellHistory(const std::string &username, bool isBuy,
        json &trade);
    bool check(const std::string &username, const std::string &method);

private:
    bool orderHashToJson(const std::unordered_map<std::string, std::string> &hmap, json &order);
    bool orderJsonToHash(const json &order, std::unordered_map<std::string, std::string> &hmap);

private:
    Redis service;
};
#endif //WORKER_REDIS_CACHE_H
