#include "RedisCache.h"

RedisCache::RedisCache(json config):
    service(config["redisConnection"].get<std::string>()),
    timeout(config["timeout"].get<int>())
{
    std::string memLimit = config["memoryLimit"].get<std::string>();
    service.command("config", "set", "maxmemory", memLimit);

    std::string eviction = config["evictionPolicy"].get<std::string>();
    service.command("config", "set", "maxmemory-policy", eviction);
}

bool RedisCache::get(const std::string &username, const std::string &method,
    json &response)
{
    auto val = service.get(key);
    if (val) {
        // Dereference val to get the returned value of std::string type.
        response = json::parse(*val);
        return true;
    }
    return false;
}

bool RedisCache::put(const std::string &username, const std::string &method,
    json &data)
{
    std::string val = data.dump();
    return service.set(key, val);
}

bool RedisCache::del(const std::string &username, const std::string &method)
{
    return service.del(key);
}

bool RedisCache::setTTL(const std::string &key)
{
    return service.expire(key, timeout);
}