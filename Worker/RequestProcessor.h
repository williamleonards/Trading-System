//
// Created by William Leonard Sumendap on 12/27/21.
//

#ifndef WORKER_REQUEST_PROCESSOR_H
#define WORKER_REQUEST_PROCESSOR_H

#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>

#include <amqpcpp.h>
#include "TradeEngine.h"
#include "RedisCache.h"
#include "utils.h"

using namespace sw::redis;
using json = nlohmann::json;

class RequestProcessor
{
public:
    RequestProcessor(TradeEngine &ts, RedisCache &cache, AMQP::Channel &channel,
        const std::string &exchange, const std::string &responseRouting);
    void processRegisterRequest(json &args);
    void processLoginRequest(json &args);
    void processBuyRequest(json &args);
    void processSellRequest(json &args);
    void processPendingBuyOrderRequest(json &args);
    void processPendingSellOrderRequest(json &args);
    void processDeleteBuyRequest(json &args);
    void processDeleteSellRequest(json &args);
    void processBuyTreeRequest(json &args);
    void processSellTreeRequest(json &args);
    void processBuyHistoryRequest(json &args);
    void processSellHistoryRequest(json &args);      
    void processUnknownRequest(json &args);
        
private:
    TradeEngine &ts;
    RedisCache &cache;
    AMQP::Channel &channel;
    const std::string &exchange;
    const std::string &responseRouting;
};
#endif //WORKER_REQUEST_PROCESSOR_H

/*
    Read operations
    1) Try to get from cache, if exists, return it.
    2) Query DB to get response.
    3) Update the cache.

    Write operations
    1) Query DB to get response.
    2) Delete cache entries that are touched on the response.
*/