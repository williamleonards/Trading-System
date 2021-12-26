#include "RedisCache.h"

RedisCache::RedisCache(json config):
    service(config["redisConnection"].get<std::string>())
{
    std::string memLimit = config["memoryLimit"].get<std::string>();
    service.command("config", "set", "maxmemory", memLimit);

    std::string eviction = config["evictionPolicy"].get<std::string>();
    service.command("config", "set", "maxmemory-policy", eviction);
}

bool RedisCache::check(const std::string &username, const std::string &method)
{
    std::string key = method + ":" + username;
    auto val = redis.get(key);
    if (val) return true;
    return false;
}

bool RedisCache::readBuySellTree(const std::string &ticker, bool isBuy,
    json &response)
{
    return false;
}

bool readPendingBuySell(const std::string &username, bool isBuy,
    json &response)
{
    std::string method = isBuy ? "pending_buy" : "pending_sell";
    std::string key = method + ":" + username;

    std::unordered_map<std::string, std::string> hash;
    service.hgetall(key, std::inserter(hash, hash.end()));

    json orders;
    // std::unordered_map<std::string, std::string> response;

    for (auto& it: hash) {
        // Do stuff
        
        std::unordered_map<std::string, std::string> ord;
        redis.hgetall(it.second, std::inserter(ord, ord.end()));

        json order;
        
        if (!orderHashToJson(ord, order))
        {
            return false;
        }

        orders.push_back(order);
    }

    response = {
        {"getPendingBuyOrdersResponse", orders}
    };

    return true;
}

bool RedisCache::newPendingBuySell(const std::string &username, bool isBuy,
    json &entry)
{

}

bool RedisCache::orderHashToJson(const std::unordered_map<std::string, std::string> &hmap, json &order)
{
    try
    {
        long long order_id = std::stoll(hmap["order_id"]);
        std::string username = hmap["username"];
        int amt = std::stoi(hmap["amount"]);
        int price = std::stoi(hmap["price"]);
        std::string ticker = hmap["ticker"];
        time_t datetimeLong = std::stoll(hmap["datetime"]);
        std::string datetime = std::string(ctime(&datetimeLong));
        if (username.length() == 0 || ticker.length() == 0 || datetime.length() == 0)
        {
            return false;
        }
        json entry = {
            {"order_id", order_id},
            {"price", price},
            {"amount", amt},
            {"price", price},
            {"ticker", ticker},
            {"datetime", datetime.substr(0, datetime.length() - 1)}
        };
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

bool RedisCache::orderJsonToHash(const json &order, std::unordered_map<std::string, std::string> &hmap)
{
    return false;
}