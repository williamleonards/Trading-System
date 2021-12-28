//
// Created by William Leonard Sumendap on 12/25/21.
//

#ifndef WORKER_REDIS_CACHE_H
#define WORKER_REDIS_CACHE_H

#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>

using namespace sw::redis;
using json = nlohmann::json;

class RedisCache
{
public:
    RedisCache(json config);
    bool get(const std::string &key, json &response);
    bool put(const std::string &key, json &data);
    bool del(const std::string &key);
    bool putTTL(const std::string &key, json &data);
    bool setTTL(const std::string &key);
private:
    Redis service;
    std::chrono::milliseconds timeout;
    int timeoutInSeconds;
};
#endif //WORKER_REDIS_CACHE_H

/*
    Read operations
    1) Try to get from cache, if exists, return it.
    2) Query DB to get response.
    3) Update the cache.

    Write operations
    1) Query DB to get response.
    2) Delete cache entries that are touched on the response.
*/