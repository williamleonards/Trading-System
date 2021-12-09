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

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
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
    int price = args["price"].get<int>();
    int amt = args["amount"].get<int>();
    std::string ticker = args["ticker"].get<string>();

    json resp = ts.placeBuyOrder(username, price, amt, ticker);
    return formResponse(reqId, resp);
}

json processSellRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    int price = args["price"].get<int>();
    int amt = args["amount"].get<int>();
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

std::string processPendingSellOrderRequest(TradeEngine &ts, json args)
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
    long long orderId = args["orderId"].get<long long>();

    json resp = ts.deleteBuyOrder(username, orderId);
    return formResponse(reqId, resp);
}

json processDeleteSellRequest(TradeEngine &ts, json args)
{
    int reqId = args["id"].get<int>();
    std::string username = args["username"].get<string>();
    long long orderId = args["orderId"].get<long long>();

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
    TradeEngine ts = TradeEngine("user=postgres password=Password12345 dbname=Trading-System hostaddr=127.0.0.1 port=5434");

    SimplePocoHandler handler("127.0.0.1", 5672);
    AMQP::Connection connection(&handler, AMQP::Login("guest", "guest"), "/");
    AMQP::Channel channel(&connection);

    channel.onReady([&]() {
        std::cout << "Worker is ready!" << std::endl;
    });

    channel.declareExchange("ts-exchange", AMQP::direct);
    channel.declareQueue("ts-generic-response");
    channel.bindQueue("ts-exchange", "ts-generic-response", "generic-response");
    channel.consume("ts-generic-request", AMQP::noack).onReceived([&channel, &ts](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::cout << "[x] Received " << message.body() << std::endl
                << std::endl;

        std::string msg = message.body();
        msg = msg.substr(0, msg.find_first_of('\n'));
        
        json args = json::parse(msg);

        std::string method = args["method"].get<string>();

        json response;

        // TODO: ADD EXCEPTION HANDLING FOR STOI PARSE EXCEPTIONS
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

        if (channel.ready())
        {
            channel.publish("ts-exchange", "generic-response", response.dump() + "\n");
        }
        else
        {
            std::cout << "Can't publish, channel unavailable" << std::endl;
        }
    });

    handler.loop();

    return 0;
}
