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

std::string toString(Order &o)
{
    std::string type = o.getType() ? "buy," : "sell,";
    return "(" + type + to_string(o.getID()) + "," + to_string(o.getIssuerID()) + "," + to_string(o.getPrice()) + "," + to_string(o.getAmt()) + ")";
}

std::string toString(Trade &t)
{
    return "(" + to_string(t.getBuyerID()) + "," + to_string(t.getSellerID()) + "," + to_string(t.getPrice()) + "," + to_string(t.getAmt()) + ")";
}

std::string toString(std::pair<int, int> &p)
{
    return "[" + to_string(p.first) + ", " + to_string(p.second) + "]";
}

template <typename T>
std::string toString(std::vector<T*> &vec)
{
    std::string ans = "";
    for (int i = 0; i < vec.size(); i++)
    {
        ans += toString(*vec[i]);
    }
    return ans;
}

template <typename T>
std::string toString(std::vector<T> &vec)
{
    std::string ans = "";
    for (int i = 0; i < vec.size(); i++)
    {
        ans += toString(vec[i]);
    }
    return ans;
}

std::string formResponse(std::string id, std::string resp)
{
    return id + "|" + resp + "|";
}

std::string processRegisterRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    std::string name = args[2];
    int userId = ts.createUser(name);
    return formResponse(reqId, to_string(userId));
}

std::string processBuyRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    int userId = stoi(args[2]);
    int price = stoi(args[3]);
    int amt = stoi(args[4]);

    std::vector<Trade *> trades = ts.placeBuyOrder(userId, price, amt);
    return formResponse(reqId, toString(trades));
}

std::string processSellRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    int userId = stoi(args[2]);
    int price = stoi(args[3]);
    int amt = stoi(args[4]);

    std::vector<Trade *> trades = ts.placeSellOrder(userId, price, amt);
    return formResponse(reqId, toString(trades));
}

std::string processPendingOrderRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    int userId = stoi(args[2]);

    std::vector<Order *> pending = ts.getPendingOrders(userId);
    return formResponse(reqId, toString(pending));
}

std::string processDeleteRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    int userId = stoi(args[2]);
    int orderId = stoi(args[3]);

    ts.deleteOrder(userId, orderId);
    return formResponse(reqId, "Delete completed, check if input is valid");
}

std::string processBuyTreeRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];

    std::vector<std::pair<int, int>> buyTree = ts.getPendingBuys();
    return formResponse(reqId, toString(buyTree));
}

std::string processSellTreeRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];

    std::vector<std::pair<int, int>> sellTree = ts.getPendingSells();
    return formResponse(reqId, toString(sellTree));
}

std::string processBuyHistoryRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    int userId = stoi(args[2]);

    std::vector<Trade*> *buyHistory = ts.getBuyTrades(userId);
    return formResponse(reqId, toString(*buyHistory));
}

std::string processSellHistoryRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];
    int userId = stoi(args[2]);

    std::vector<Trade*> *sellHistory = ts.getSellTrades(userId);
    return formResponse(reqId, toString(*sellHistory));
}

std::string processUnknownRequest(TradeEngine &ts, std::vector<std::string> args)
{
    std::string reqId = args[0];

    return formResponse(reqId, "Unknown request");
}

int main()
{
    TradeEngine ts = TradeEngine();

    SimplePocoHandler handler("127.0.0.1", 5672);
    AMQP::Connection connection(&handler, AMQP::Login("guest", "guest"), "/");
    AMQP::Channel channel(&connection);

    channel.onReady([&]() {
        std::cout << "Worker is ready!" << std::endl;

    });

    channel.declareExchange("ts-exchange", AMQP::direct);
    channel.declareQueue("ts-generic-response");
    channel.bindQueue("ts-exchange", "ts-generic-response", "generic-response");
    channel.consume("ts-generic-request", AMQP::noack).onReceived(
            [&channel, &ts](const AMQP::Message &message,
                   uint64_t deliveryTag,
                   bool redelivered)
            {
                std::cout << "[x] Received " << message.body() << std::endl << std::endl;

                std::string msg = message.body();
                std::vector<std::string> tokens;
                boost::algorithm::split(tokens, msg, boost::algorithm::is_any_of("|"));

//                int id = std::stoi(method);
//                std::string n = tokens[1];
//
//                std::cout << "Request id is " << id << " and n is " << n << std::endl;
//
//                std::this_thread::sleep_for(std::chrono::milliseconds(200));
//
//                std::string response = std::to_string(id) + "|" + n + "|";
//                std::cout << "Response is " << response << std::endl;
//
//                if (channel.ready())
//                {
//                    channel.publish("ts-exchange", "generic-response", response);
//                } else {
//                    std::cout << "Can't publish, channel unavailable" << std::endl;
//                }

                std::string method = tokens[1];

                std::string response;

                if (method == "register")
                {
                    response = processRegisterRequest(ts, tokens);
                }
                else if (method == "delete")
                {
                    response = processDeleteRequest(ts, tokens);
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
                    response = processBuyTreeRequest(ts, tokens);
                }
                else if (method == "sell-tree")
                {
                    response = processSellTreeRequest(ts, tokens);
                }
                else if (method == "pending")
                {
                    response = processPendingOrderRequest(ts, tokens);
                }
                else if (method == "buy-history")
                {
                    response = processBuyHistoryRequest(ts, tokens);
                }
                else if (method == "sell-history")
                {
                    response = processSellHistoryRequest(ts, tokens);
                }
                else {
                    response = processUnknownRequest(ts, tokens);
                }

                if (channel.ready())
                {
                    channel.publish("ts-exchange", "generic-response", response);
                } else {
                    std::cout << "Can't publish, channel unavailable" << std::endl;
                }
            });

    handler.loop();

    return 0;
}
