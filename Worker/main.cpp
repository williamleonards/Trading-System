/*
  stdlib location: -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1
  
  COMPILE USING:
  g++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp -lamqpcpp -lpoconet -lpocofoundation

  g++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp TradeEngine.cpp -lpqxx -lpq -lamqpcpp -lpoconet -lpocofoundation -lredis++ -lhiredis -pthread -lbcryptcpp
  
  /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp Order.cpp Trade.cpp User.cpp TradeEngine.cpp -lamqpcpp -lpoconet -lpocofoundation
  
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>

#include <nlohmann/json.hpp>

#include "SimplePocoHandler.h"
#include "TradeEngine.h"

using json = nlohmann::json;

json formResponse(int id, json resp)
{
    json workerResponse;
    workerResponse["id"] = id;
    workerResponse["response"] = resp;
    return workerResponse;
}

json processRegisterRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string name = args["username"].get<string>();
    std::string psw = args["password"].get<string>();
    json resp = ts.createUser(name, psw);
    return formResponse(reqId, resp);
}

json processLoginRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string name = args["username"].get<string>();
    std::string psw = args["password"].get<string>();
    json resp = ts.loginUser(name, psw);
    return formResponse(reqId, resp);
}

json processBuyRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    int price = std::stoi(args["price"].get<std::string>());
    int amt = std::stoi(args["amount"].get<std::string>());
    std::string ticker = args["ticker"].get<string>();

    json resp = ts.placeBuyOrder(username, price, amt, ticker);
    return formResponse(reqId, resp);
}

json processSellRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    int price = std::stoi(args["price"].get<std::string>());
    int amt = std::stoi(args["amount"].get<std::string>());
    std::string ticker = args["ticker"].get<string>();

    json resp = ts.placeSellOrder(username, price, amt, ticker);
    return formResponse(reqId, resp);
}

json processPendingBuyOrderRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    string username = args["username"].get<string>();

    json resp = ts.getPendingBuyOrders(username);
    return formResponse(reqId, resp);
}

json processPendingSellOrderRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    string username = args["username"].get<string>();

    json resp = ts.getPendingSellOrders(username);
    return formResponse(reqId, resp);
}

json processDeleteBuyRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    long long orderId = std::stoll(args["orderId"].get<std::string>());

    json resp = ts.deleteBuyOrder(username, orderId);
    return formResponse(reqId, resp);
}

json processDeleteSellRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    long long orderId = std::stoll(args["orderId"].get<std::string>());

    json resp = ts.deleteSellOrder(username, orderId);
    return formResponse(reqId, resp);
}

json processBuyVolumeRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string ticker = args["ticker"].get<string>();

    json resp = ts.getBuyVolumes(ticker);
    return formResponse(reqId, resp);
}

json processSellTreeRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string ticker = args["ticker"].get<string>();

    json resp = ts.getSellVolumes(ticker);
    return formResponse(reqId, resp);
}

json processBuyHistoryRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();

    json resp = ts.getBuyTrades(username);
    return formResponse(reqId, resp);
}

json processSellHistoryRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();

    json resp = ts.getSellTrades(username);
    return formResponse(reqId, resp);
}

json processUnknownRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();

    return formResponse(reqId, {{"unknownResponse", {}}});
}

int main()
{
    // READ CONFIG JSON
    std::ifstream file("worker-config.json"); // PATH RELATIVE TO EXECUTABLE, NOT SOURCE CODE
    std::stringstream buffer;
    buffer << file.rdbuf();
    json workerConfig = json::parse(buffer.str());

    TradeEngine ts = TradeEngine(workerConfig["dbConfig"]["credentials"].get<std::string>());

    // Extract config json
    std::string host(workerConfig["mqConfig"]["host"].get<std::string>());
    int port(workerConfig["mqConfig"]["port"].get<int>());
    std::string user(workerConfig["mqConfig"]["user"].get<std::string>());
    std::string password(workerConfig["mqConfig"]["password"].get<std::string>());
    std::string vhost(workerConfig["mqConfig"]["vhost"].get<std::string>());
    std::string exchange(workerConfig["mqConfig"]["exchange"].get<std::string>());
    std::string requestQueue(workerConfig["mqConfig"]["requestQueue"].get<std::string>());
    std::string requestRouting(workerConfig["mqConfig"]["requestRouting"].get<std::string>());
    std::string responseQueue(workerConfig["mqConfig"]["responseQueue"].get<std::string>());
    std::string responseRouting(workerConfig["mqConfig"]["responseRouting"].get<std::string>());

    SimplePocoHandler handler(host, port);
    AMQP::Connection connection(&handler, AMQP::Login(user, password), vhost);
    AMQP::Channel channel(&connection);

    channel.onReady([&]() {
        std::cout << "Worker is ready!" << std::endl;
    });

    channel.declareExchange(exchange, AMQP::direct);
    channel.declareQueue(responseQueue);
    channel.bindQueue(exchange, responseQueue, responseRouting);
    channel.consume(requestQueue, AMQP::noack).onReceived([&channel, &ts, &exchange, &responseRouting](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "[x] Received " << message.body() << std::endl
                << std::endl;

        std::string msg = message.body();
        msg = msg.substr(0, msg.find_first_of('\n'));
        
        json args = json::parse(msg);

        std::string method = args["method"].get<string>();

        json response;

        try
        {
            if (method == "register")
            {
                response = processRegisterRequest(ts, args);
            }
            else if (method == "login")
            {
                response = processLoginRequest(ts, args);
            }
            else if (method == "delete-buy")
            {
                response = processDeleteBuyRequest(ts, args);
            }
            else if (method == "delete-sell")
            {
                response = processDeleteSellRequest(ts, args);
            }
            else if (method == "buy")
            {
                response = processBuyRequest(ts, args);
            }
            else if (method == "sell")
            {
                response = processSellRequest(ts, args);
            }
            else if (method == "buy-tree")
            {
                response = processBuyVolumeRequest(ts, args);
            }
            else if (method == "sell-tree")
            {
                response = processSellTreeRequest(ts, args);
            }
            else if (method == "pending-buy")
            {
                response = processPendingBuyOrderRequest(ts, args);
            }
            else if (method == "pending-sell")
            {
                response = processPendingSellOrderRequest(ts, args);
            }
            else if (method == "buy-history")
            {
                response = processBuyHistoryRequest(ts, args);
            }
            else if (method == "sell-history")
            {
                response = processSellHistoryRequest(ts, args);
            }
            else
            {
                response = processUnknownRequest(ts, args);
            }
        }
        catch (const std::exception &e)
        {
            int id = args["id"].get<int>();
            response = {
                {"response", {
                    {"error", "Error parsing input arguments"}
                }},
                {"id", id}
            };
        }

        if (channel.ready())
        {
            channel.publish(exchange, responseRouting, response.dump() + "\n");
        }
        else
        {
            std::cout << "Can't publish, channel unavailable" << std::endl;
        }
    });

    handler.loop();

    return 0;
}
