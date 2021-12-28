/*
  stdlib location: -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1
  
  COMPILE USING:
  g++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp -lamqpcpp -lpoconet -lpocofoundation

  g++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp TradeEngine.cpp RedisCache.cpp RequestProcessor.cpp utils.cpp -lpqxx -lpq -lamqpcpp -lpoconet -lpocofoundation -lredis++ -lhiredis -pthread -lbcryptcpp
  
  /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp Order.cpp Trade.cpp User.cpp TradeEngine.cpp -lamqpcpp -lpoconet -lpocofoundation
  
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>

#include <nlohmann/json.hpp>

#include "SimplePocoHandler.h"
#include "TradeEngine.h"
#include "RedisCache.h"
#include "RequestProcessor.h"
// #include "utils.cpp"

// void processRegisterRequest(TradeEngine &ts, json &args, RedisCache &cache, AMQP::Channel &channel,
//     const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string name = args["username"].get<string>();
//     std::string psw = args["password"].get<string>();
//     json response = ts.createUser(name, psw);
//     response = formResponse(reqId, response);
//     sendResponse(response, channel, exchange, responseRouting);
// }

// void processLoginRequest(TradeEngine &ts, json &args, RedisCache &cache, AMQP::Channel &channel,
//     const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string name = args["username"].get<string>();
//     std::string psw = args["password"].get<string>();
//     json response = ts.loginUser(name, psw);
//     response = formResponse(reqId, response);
//     sendResponse(response, channel, exchange, responseRouting);
// }

// void processBuyRequest(TradeEngine &ts, json &args, RedisCache &cache, AMQP::Channel &channel,
//     const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string username = args["username"].get<string>();
//     int price = std::stoi(args["price"].get<std::string>());
//     int amt = std::stoi(args["amount"].get<std::string>());
//     std::string ticker = args["ticker"].get<string>();

//     json response = ts.placeBuyOrder(username, price, amt, ticker);
//     response = formResponse(reqId, response);
//     sendResponse(response, channel, exchange, responseRouting);

//     json trades = response["response"]["placeBuyOrderResponse"];
//     if (trades.is_null()) return;
//     for (int i = 0; i < trades.size(); i++) {
//         std::string counterparty = trades[i]["seller"];
//         cache.del("pending_sell:" + counterparty);
//         cache.del("sell_history:" + counterparty);
//     }
//     cache.del("pending_buy:" + username);
//     cache.del("buy_history:" + username);
// }

// void processSellRequest(TradeEngine &ts, json &args, RedisCache &cache, AMQP::Channel &channel,
//     const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string username = args["username"].get<string>();
//     int price = std::stoi(args["price"].get<std::string>());
//     int amt = std::stoi(args["amount"].get<std::string>());
//     std::string ticker = args["ticker"].get<string>();

//     json response = ts.placeSellOrder(username, price, amt, ticker);
//     response = formResponse(reqId, response);
//     sendResponse(response, channel, exchange, responseRouting);

//     json trades = response["response"]["placeSellOrderResponse"];
//     if (trades.is_null()) return;
//     for (int i = 0; i < trades.size(); i++) {
//         std::string counterparty = trades[i]["seller"];
//         cache.del("pending_buy:" + counterparty);
//         cache.del("buy_history:" + counterparty);
//     }
//     cache.del("pending_sell:" + username);
//     cache.del("sell_history:" + username);
// }

// void processPendingBuyOrderRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     string username = args["username"].get<string>();

//     json response;
//     std::string key = "pending_buy:" + username;

//     if (cache.get(key, response)) 
//     {
//         response["id"] = reqId;
//         sendResponse(response, channel, exchange, responseRouting);
//         return;
//     }

//     response = ts.getPendingBuyOrders(username);
//     response = formResponse(reqId, response);

//     sendResponse(response, channel, exchange, responseRouting);

//     cache.put(key, response);
// }

// void processPendingSellOrderRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     string username = args["username"].get<string>();

//     json response;
//     std::string key = "pending_sell:" + username;

//     if (cache.get(key, response)) 
//     {
//         response["id"] = reqId;
//         sendResponse(response, channel, exchange, responseRouting);
//         return;
//     }

//     response = ts.getPendingSellOrders(username);
//     response = formResponse(reqId, response);

//     sendResponse(response, channel, exchange, responseRouting);

//     cache.put(key, response);
// }

// void processDeleteBuyRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string username = args["username"].get<string>();
//     long long orderId = std::stoll(args["orderId"].get<std::string>());

//     json response = ts.deleteBuyOrder(username, orderId);
//     response = formResponse(reqId, response);
//     sendResponse(response, channel, exchange, responseRouting);

//     cache.del("pending_buy:" + username);
// }

// void processDeleteSellRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string username = args["username"].get<string>();
//     long long orderId = std::stoll(args["orderId"].get<std::string>());

//     json response = ts.deleteSellOrder(username, orderId);
//     response = formResponse(reqId, response);
//     sendResponse(response, channel, exchange, responseRouting);

//     cache.del("pending_sell:" + username);
// }

// void processBuyTreeRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string ticker = args["ticker"].get<string>();
    
//     json response;
//     std::string key = "buy_tree:" + ticker;

//     if (cache.get(key, response)) 
//     {
//         response["id"] = reqId;
//         sendResponse(response, channel, exchange, responseRouting);
//         return;
//     }

//     response = ts.getBuyVolumes(ticker);
//     response = formResponse(reqId, response);

//     sendResponse(response, channel, exchange, responseRouting);

//     cache.putTTL(key, response);
// }

// void processSellTreeRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string ticker = args["ticker"].get<string>();

//     json response;
//     std::string key = "sell_tree:" + ticker;

//     if (cache.get(key, response)) 
//     {
//         response["id"] = reqId;
//         sendResponse(response, channel, exchange, responseRouting);
//         return;
//     }

//     response = ts.getSellVolumes(ticker);
//     response = formResponse(reqId, response);

//     sendResponse(response, channel, exchange, responseRouting);

//     cache.putTTL(key, response);
// }

// void processBuyHistoryRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string username = args["username"].get<string>();

//     json response;
//     std::string key = "buy_history:" + username;

//     if (cache.get(key, response)) 
//     {
//         response["id"] = reqId;
//         sendResponse(response, channel, exchange, responseRouting);
//         return;
//     }

//     response = ts.getBuyTrades(username);
//     response = formResponse(reqId, response);

//     sendResponse(response, channel, exchange, responseRouting);

//     cache.put(key, response);
// }

// void processSellHistoryRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();
//     std::string username = args["username"].get<string>();

//     json response;
//     std::string key = "sell_history:" + username;

//     if (cache.get(key, response)) 
//     {
//         response["id"] = reqId;
//         sendResponse(response, channel, exchange, responseRouting);
//         return;
//     }

//     response = ts.getSellTrades(username);
//     response = formResponse(reqId, response);

//     sendResponse(response, channel, exchange, responseRouting);

//     cache.put(key, response);
// }

// void processUnknownRequest(TradeEngine &ts, json &args, RedisCache &cache,
//     AMQP::Channel &channel, const std::string &exchange, const std::string &responseRouting)
// {
//     int reqId = args["id"].get<int>();

//     json response = formResponse(reqId, {{"unknownResponse", {}}});

//     sendResponse(response, channel, exchange, responseRouting);
// }

int main()
{
    // READ CONFIG JSON
    std::ifstream file("worker-config.json"); // PATH RELATIVE TO EXECUTABLE, NOT SOURCE CODE
    std::stringstream buffer;
    buffer << file.rdbuf();
    json workerConfig = json::parse(buffer.str());

    TradeEngine ts(workerConfig["dbConfig"]["credentials"].get<std::string>());

    // Extract config json
    json mqConfig = workerConfig["mqConfig"];
    std::string host(mqConfig["host"].get<std::string>());
    int port(mqConfig["port"].get<int>());
    std::string user(mqConfig["user"].get<std::string>());
    std::string password(mqConfig["password"].get<std::string>());
    std::string vhost(mqConfig["vhost"].get<std::string>());
    std::string exchange(mqConfig["exchange"].get<std::string>());
    std::string requestQueue(mqConfig["requestQueue"].get<std::string>());
    std::string requestRouting(mqConfig["requestRouting"].get<std::string>());
    std::string responseQueue(mqConfig["responseQueue"].get<std::string>());
    std::string responseRouting(mqConfig["responseRouting"].get<std::string>());

    RedisCache cache(workerConfig["redisCacheConfig"]);

    SimplePocoHandler handler(host, port);
    AMQP::Connection connection(&handler, AMQP::Login(user, password), vhost);
    AMQP::Channel channel(&connection);

    channel.onReady([&]() {
        std::cout << "Worker is ready!" << std::endl;
    });

    channel.declareExchange(exchange, AMQP::direct);
    channel.declareQueue(responseQueue);
    channel.bindQueue(exchange, responseQueue, responseRouting);

    RequestProcessor rp(ts, cache, channel, exchange, responseRouting);

    channel.consume(requestQueue, AMQP::noack).onReceived([&channel, &ts, &exchange, &cache, &rp,
        &responseRouting](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "[x] Received " << message.body() << std::endl
                << std::endl;

        std::string msg = message.body();
        msg = msg.substr(0, msg.find_first_of('\n'));
        
        json args = json::parse(msg);

        std::string method = args["method"].get<string>();

        /* ROUTER CLASS!!!
           CACHE OPERATIONS HERE
           For read operations, cache operations inside process method
           For write operations, cache operations after response is sent
           Inject channel & cache dependencies into process methods
        */
        try
        {
            if (method == "register")
            {
                rp.processRegisterRequest(args);
            }
            else if (method == "login")
            {
                rp.processLoginRequest(args);
            }
            else if (method == "delete-buy")
            {
                rp.processDeleteBuyRequest(args);
            }
            else if (method == "delete-sell")
            {
                rp.processDeleteSellRequest(args);
            }
            else if (method == "buy")
            {
                rp.processBuyRequest(args);
            }
            else if (method == "sell")
            {
                rp.processSellRequest(args);
            }
            else if (method == "buy-tree")
            {
                rp.processBuyTreeRequest(args);
            }
            else if (method == "sell-tree")
            {
                rp.processSellTreeRequest(args);
            }
            else if (method == "pending-buy")
            {
                rp.processPendingBuyOrderRequest(args);
            }
            else if (method == "pending-sell")
            {
                rp.processPendingSellOrderRequest(args);
            }
            else if (method == "buy-history")
            {
                rp.processBuyHistoryRequest(args);
            }
            else if (method == "sell-history")
            {
                rp.processSellHistoryRequest(args);
            }
            else
            {
                rp.processUnknownRequest(args);
            }
        }
        catch (const std::exception &e)
        {
            int id = args["id"].get<int>();
            json response = {
                {"response", {
                    {"error", "Error parsing input arguments"}
                }},
                {"id", id}
            };
            sendResponse(response, channel, exchange, responseRouting);
        }
    });

    handler.loop();

    return 0;
}