/*
  stdlib location: -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1
  
  COMPILE USING:
  g++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp -lamqpcpp -lpoconet -lpocofoundation

  g++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp TradeEngine.cpp -lpqxx -lpq -lamqpcpp -lpoconet -lpocofoundation -lredis++ -lhiredis -pthread
  
  /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ -Werror -std=c++17 main.cpp SimplePocoHandler.cpp Order.cpp Trade.cpp User.cpp TradeEngine.cpp -lamqpcpp -lpoconet -lpocofoundation
  
*/

#include <iostream>
#include <thread>
#include <chrono>

#include "SimplePocoHandler.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "TradeEngine.h"

/*
 * Request string encoding: <request-id>|<method-name>|<args>...
 */

std::string formResponse(std::string id, std::string resp)
{
    return id + "|" + resp + "|";
}

std::string processRegisterRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string name = args[2];
    std::string psw = args[3];
    std::string resp = ts.createUser(name, psw);
    return formResponse(reqId, resp);
}

std::string processLoginRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string name = args[2];
    std::string psw = args[3];
    std::string resp = ts.loginUser(name, psw);
    return formResponse(reqId, resp);
}

std::string processBuyRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string username = args[2];
    int price = stoi(args[3]);
    int amt = stoi(args[4]);
    std::string ticker = args[5];

    std::string resp = ts.placeBuyOrder(username, price, amt, ticker);
    return formResponse(reqId, resp);
}

std::string processSellRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string username = args[2];
    int price = stoi(args[3]);
    int amt = stoi(args[4]);
    std::string ticker = args[5];

    std::string resp = ts.placeSellOrder(username, price, amt, ticker);
    return formResponse(reqId, resp);
}

std::string processPendingBuyOrderRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    string username = args[2];

    std::string resp = ts.getPendingBuyOrders(username);
    return formResponse(reqId, resp);
}

std::string processPendingSellOrderRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    string username = args[2];

    std::string resp = ts.getPendingSellOrders(username);
    return formResponse(reqId, resp);
}

std::string processDeleteBuyRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string username = args[2];
    long long orderId = stoll(args[3]);

    std::string resp = ts.deleteBuyOrder(username, orderId);
    return formResponse(reqId, resp);
}

std::string processDeleteSellRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string username = args[2];
    long long orderId = stoll(args[3]);

    std::string resp = ts.deleteSellOrder(username, orderId);
    return formResponse(reqId, resp);
}

std::string processBuyVolumeRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string ticker = args[2];

    std::string resp = ts.getBuyVolumes(ticker);
    return formResponse(reqId, resp);
}

std::string processSellTreeRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string ticker = args[2];

    std::string resp = ts.getSellVolumes(ticker);
    return formResponse(reqId, resp);
}

std::string processBuyHistoryRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string username = args[2];

    std::string resp = ts.getBuyTrades(username);
    return formResponse(reqId, resp);
}

std::string processSellHistoryRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string username = args[2];

    std::string resp = ts.getSellTrades(username);
    return formResponse(reqId, resp);
}

std::string processUnknownRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];

    return formResponse(reqId, "Unknown request");
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
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, msg, boost::algorithm::is_any_of("|"));

        std::string method = tokens[1];

        std::string response;

        // TODO: ADD EXCEPTION HANDLING FOR STOI PARSE EXCEPTIONS
        if (method == "register")
        {
            response = processRegisterRequest(ts, tokens);
        }
        else if (method == "login")
        {
            response = processLoginRequest(ts, tokens);
        }
        else if (method == "delete-buy")
        {
            response = processDeleteBuyRequest(ts, tokens);
        }
        else if (method == "delete-sell")
        {
            response = processDeleteSellRequest(ts, tokens);
        }
        else if (method == "buy")
        {
            response = processBuyRequest(ts, tokens);
        }
        else if (method == "sell")
        {
            response = processSellRequest(ts, tokens);
        }
        else if (method == "buy-tree")
        {
            response = processBuyVolumeRequest(ts, tokens);
        }
        else if (method == "sell-tree")
        {
            response = processSellTreeRequest(ts, tokens);
        }
        else if (method == "pending-buy")
        {
            response = processPendingBuyOrderRequest(ts, tokens);
        }
        else if (method == "pending-sell")
        {
            response = processPendingSellOrderRequest(ts, tokens);
        }
        else if (method == "buy-history")
        {
            response = processBuyHistoryRequest(ts, tokens);
        }
        else if (method == "sell-history")
        {
            response = processSellHistoryRequest(ts, tokens);
        }
        else
        {
            response = processUnknownRequest(ts, tokens);
        }

        if (channel.ready())
        {
        channel.publish("ts-exchange", "generic-response", response);
        }
        else
        {
        std::cout << "Can't publish, channel unavailable" << std::endl;
        }
    });

    handler.loop();

    return 0;
}
