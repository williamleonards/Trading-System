#include "RedisCache.h"
#include <iostream>

RedisCache::RedisCache(json config):
    service(config["redisConnection"].get<std::string>()),
    timeout(std::chrono::milliseconds(config["timeout"].get<int>())),
    timeoutInSeconds(config["timeout"].get<int>() / 1000)
{
    std::string memLimit = config["memoryLimit"].get<std::string>();
    service.command("config", "set", "maxmemory", memLimit);

    std::string eviction = config["evictionPolicy"].get<std::string>();
    service.command("config", "set", "maxmemory-policy", eviction);
}

bool RedisCache::get(const std::string &key, json &response)
{
    auto val = service.get(key);
    if (val) {
        // Dereference val to get the returned value of std::string type.
        response = json::parse(*val);
        return true;
    }
    return false;
}

bool RedisCache::put(const std::string &key, json &data)
{
    std::string val = data.dump();
    return service.set(key, val);
}

bool RedisCache::putTTL(const std::string &key, json &data)
{
    std::string val = data.dump();
    return service.set(key, val, timeout);
}

bool RedisCache::del(const std::string &key)
{
    return service.del(key);
}

bool RedisCache::setTTL(const std::string &key)
{
    return service.expire(key, timeoutInSeconds);
}