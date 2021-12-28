#include "RequestProcessor.h"

RequestProcessor::RequestProcessor(TradeEngine &ts, RedisCache &cache, AMQP::Channel &channel,
    const std::string &exchange, const std::string &responseRouting): ts(ts),
    cache(cache), channel(channel), exchange(exchange), responseRouting(responseRouting)
{
}

void RequestProcessor::processRegisterRequest(json &args)    
{
    int reqId = args["id"].get<int>();
    std::string name = args["username"].get<string>();
    std::string psw = args["password"].get<string>();
    json response = ts.createUser(name, psw);
    response = formResponse(reqId, response);
    sendResponse(response, channel, exchange, responseRouting);
}

void RequestProcessor::processLoginRequest(json &args)
{
    int reqId = args["id"].get<int>();
    std::string name = args["username"].get<string>();
    std::string psw = args["password"].get<string>();
    json response = ts.loginUser(name, psw);
    response = formResponse(reqId, response);
    sendResponse(response, channel, exchange, responseRouting);
}

void RequestProcessor::processBuyRequest(json &args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    int price = std::stoi(args["price"].get<std::string>());
    int amt = std::stoi(args["amount"].get<std::string>());
    std::string ticker = args["ticker"].get<string>();

    json response = ts.placeBuyOrder(username, price, amt, ticker);
    response = formResponse(reqId, response);
    sendResponse(response, channel, exchange, responseRouting);

    json trades = response["response"]["placeBuyOrderResponse"];
    if (trades.is_null()) return;
    for (int i = 0; i < trades.size(); i++) {
        std::string counterparty = trades[i]["seller"];
        cache.del("pending_sell:" + counterparty);
        cache.del("sell_history:" + counterparty);
    }
    cache.del("pending_buy:" + username);
    cache.del("buy_history:" + username);
}

void RequestProcessor::processSellRequest(json &args) 
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    int price = std::stoi(args["price"].get<std::string>());
    int amt = std::stoi(args["amount"].get<std::string>());
    std::string ticker = args["ticker"].get<string>();

    json response = ts.placeSellOrder(username, price, amt, ticker);
    response = formResponse(reqId, response);
    sendResponse(response, channel, exchange, responseRouting);

    json trades = response["response"]["placeSellOrderResponse"];
    if (trades.is_null()) return;
    for (int i = 0; i < trades.size(); i++) {
        std::string counterparty = trades[i]["seller"];
        cache.del("pending_buy:" + counterparty);
        cache.del("buy_history:" + counterparty);
    }
    cache.del("pending_sell:" + username);
    cache.del("sell_history:" + username);
}

void RequestProcessor::processPendingBuyOrderRequest(json &args)   
{
    int reqId = args["id"].get<int>();
    string username = args["username"].get<string>();

    json response;
    std::string key = "pending_buy:" + username;

    if (cache.get(key, response)) 
    {
        response["id"] = reqId;
        sendResponse(response, channel, exchange, responseRouting);
        return;
    }

    response = ts.getPendingBuyOrders(username);
    response = formResponse(reqId, response);

    sendResponse(response, channel, exchange, responseRouting);

    cache.put(key, response);
}

void RequestProcessor::processPendingSellOrderRequest(json &args)   
{
    int reqId = args["id"].get<int>();
    string username = args["username"].get<string>();

    json response;
    std::string key = "pending_sell:" + username;

    if (cache.get(key, response)) 
    {
        response["id"] = reqId;
        sendResponse(response, channel, exchange, responseRouting);
        return;
    }

    response = ts.getPendingSellOrders(username);
    response = formResponse(reqId, response);

    sendResponse(response, channel, exchange, responseRouting);

    cache.put(key, response);
}

void RequestProcessor::processDeleteBuyRequest(json &args)   
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    long long orderId = std::stoll(args["orderId"].get<std::string>());

    json response = ts.deleteBuyOrder(username, orderId);
    response = formResponse(reqId, response);
    sendResponse(response, channel, exchange, responseRouting);

    cache.del("pending_buy:" + username);
}

void RequestProcessor::processDeleteSellRequest(json &args)    
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    long long orderId = std::stoll(args["orderId"].get<std::string>());

    json response = ts.deleteSellOrder(username, orderId);
    response = formResponse(reqId, response);
    sendResponse(response, channel, exchange, responseRouting);

    cache.del("pending_sell:" + username);
}

void RequestProcessor::processBuyTreeRequest(json &args)
{
    int reqId = args["id"].get<int>();
    std::string ticker = args["ticker"].get<string>();
    
    json response;
    std::string key = "buy_tree:" + ticker;

    if (cache.get(key, response)) 
    {
        response["id"] = reqId;
        sendResponse(response, channel, exchange, responseRouting);
        return;
    }

    response = ts.getBuyVolumes(ticker);
    response = formResponse(reqId, response);

    sendResponse(response, channel, exchange, responseRouting);

    cache.putTTL(key, response);
}

void RequestProcessor::processSellTreeRequest(json &args)   
{
    int reqId = args["id"].get<int>();
    std::string ticker = args["ticker"].get<string>();

    json response;
    std::string key = "sell_tree:" + ticker;

    if (cache.get(key, response)) 
    {
        response["id"] = reqId;
        sendResponse(response, channel, exchange, responseRouting);
        return;
    }

    response = ts.getSellVolumes(ticker);
    response = formResponse(reqId, response);

    sendResponse(response, channel, exchange, responseRouting);

    cache.putTTL(key, response);
}

void RequestProcessor::processBuyHistoryRequest(json &args)   
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();

    json response;
    std::string key = "buy_history:" + username;

    if (cache.get(key, response)) 
    {
        response["id"] = reqId;
        sendResponse(response, channel, exchange, responseRouting);
        return;
    }

    response = ts.getBuyTrades(username);
    response = formResponse(reqId, response);

    sendResponse(response, channel, exchange, responseRouting);

    cache.put(key, response);
}

void RequestProcessor::processSellHistoryRequest(json &args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();

    json response;
    std::string key = "sell_history:" + username;

    if (cache.get(key, response)) 
    {
        response["id"] = reqId;
        sendResponse(response, channel, exchange, responseRouting);
        return;
    }

    response = ts.getSellTrades(username);
    response = formResponse(reqId, response);

    sendResponse(response, channel, exchange, responseRouting);

    cache.put(key, response);
}

void RequestProcessor::processUnknownRequest(json &args)  
{
    int reqId = args["id"].get<int>();

    json response = formResponse(reqId, {{"unknownResponse", {}}});

    sendResponse(response, channel, exchange, responseRouting);
}