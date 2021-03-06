//
// Created by William Leonard Sumendap on 7/10/20.
//

#include "TradeEngine.h"
#include <pthread.h>
#include <bcrypt.h>

bool validateTicker(string ticker)
{
    if (ticker.length() > 5) return false;
    for (int i = 0; i < ticker.length(); i++)
    {
        if (!std::isalnum(ticker[i])) return false;
    }
    return true;
}

TradeEngine::TradeEngine(string conn) : C(conn)
{
    prepareStatements();
}

json TradeEngine::createUser(string name, string password)
{
    pqxx::work W{C};
    json response;
    try
    {
        // try catch sufficient, no need to check result
        W.exec_prepared("createUser", name, hashPassword(password));
        W.commit();
        response = {
            {"createUserResponse", {}}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::loginUser(string username, string password)
{
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("getUserPassword", username)};
        W.commit();
        if (R.size() == 0) 
        {
            response = {
                {"loginUserResponse", false}
            };
        }
        else
        {
            string hash = R[0][0].as<string>();
            response = {
                {"loginUserResponse", bcrypt::validatePassword(password,hash)}
            };
        }
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::deleteBuyOrder(string username, long long orderId)
{
    //lazy deletion (?)
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("deleteBuyOrder", orderId, username)};
        W.commit();
        response = {
            {"deleteBuyOrderResponse", R.size() > 0}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::deleteSellOrder(string username, long long orderId)
{
    //lazy deletion (?)
    pqxx::work W{C};
    json response;
    try
    {
        W.exec_prepared("deleteSellOrder", orderId, username);
        W.commit();
        response = {
            {"deleteSellOrderResponse", {}}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::getBuyVolumes(string ticker)
{
    pqxx::work W{C};
    json response;
    try
    {
        if (!validateTicker(ticker))
        {
            throw std::invalid_argument("invalid ticker format");
        }
        pqxx::result R{W.exec_prepared("getBuyVolumes", ticker)};
        json entries;
        for (auto row : R)
        {
            int price = row[0].as<int>();
            int vol = row[1].as<int>();
            json entry = {
                {"price", price},
                {"volume", vol}
            };
            entries.push_back(entry);
        }
        response = {
            {"getBuyVolumesResponse", {
                {"ticker", ticker},
                {"entries", entries}
            }}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::getSellVolumes(string ticker)
{
    pqxx::work W{C};
    json response;
    try
    {
        if (!validateTicker(ticker))
        {
            throw std::invalid_argument("invalid ticker format");
        }
        pqxx::result R{W.exec_prepared("getSellVolumes", ticker)};
        json entries;
        for (auto row : R)
        {
            int price = row[0].as<int>();
            int vol = row[1].as<int>();
            json entry = {
                {"price", price},
                {"volume", vol}
            };
            entries.push_back(entry);
        }
        response = {
            {"getBuyVolumesResponse", {
                {"ticker", ticker},
                {"entries", entries}
            }}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::placeBuyOrder(string buyer, int price, int amt, string ticker)
{
    json response;
    try
    {
        if (price <= 0 || amt <= 0)
        {
            throw std::invalid_argument("received negative value for price/amt");
        }

        if (!validateTicker(ticker))
        {
            throw std::invalid_argument("invalid ticker format");
        }

        int currAmt = amt;
        vector<pair<long long, int>> partiallyConvertedOrders;
        json trades;
        
        pqxx::work W{C};
        pqxx::result R{W.exec_prepared("getOrdersWithLowestPrice", ticker, price)};
        
        while (R.size() > 0 && currAmt > 0)
        {
            int currPrice = R[0][3].as<int>();
            for (auto row : R)
            {
                long long order_id = row[0].as<long long>();
                string seller = row[1].as<string>();
                int amt = row[2].as<int>();
                int sellPrice = row[3].as<int>();
                long long datetime = std::time(nullptr);
                if (currAmt >= amt)
                {
                    json trade = {
                        {"price", sellPrice},
                        {"amount", amt},
                        {"buyer", buyer},
                        {"seller", seller},
                        {"ticker", ticker},
                        {"datetime", datetime}
                    };
                    trades.push_back(trade);
                    currAmt -= amt;
                }
                else
                {
                    json trade = {
                        {"price", sellPrice},
                        {"amount", currAmt},
                        {"buyer", buyer},
                        {"seller", seller},
                        {"ticker", ticker},
                        {"datetime", datetime}
                    };
                    partiallyConvertedOrders.push_back(make_pair(order_id, amt - currAmt));
                    trades.push_back(trade);
                    currAmt = 0;
                    break;
                }
            }
            // delete fully converted orders
            if (partiallyConvertedOrders.size() > 0)
            {
                W.exec_prepared("deleteSellOrdersAtPrice", currPrice, partiallyConvertedOrders[0].first);
            }
            else
            {
                W.exec_prepared("deleteAllSellOrdersAtPrice", currPrice);
            }
            
            R = {W.exec_prepared("getOrdersWithLowestPrice", ticker, price)};
        }
        // update partially converted order (if any)
        for (int i = 0; i < partiallyConvertedOrders.size(); i++)
        {
            ostringstream updateQuery;
            int amountLeft = partiallyConvertedOrders[i].second;
            long long order_id = partiallyConvertedOrders[i].first;

            W.exec_prepared("updatePartialSellOrder", amountLeft, order_id);
        }
        // insert newly created trades
        for (int i = 0; i < trades.size(); i++)
        {
            json &trade = trades[i];
            
            int tradePrice = trade["price"].get<int>();
            int tradeAmount = trade["amount"].get<int>();
            string tradeBuyer = trade["buyer"].get<string>();
            string tradeSeller = trade["seller"].get<string>();
            long timeNow = trade["datetime"].get<long>();
            W.exec_prepared("insertTrades", tradeAmount, tradePrice, tradeBuyer, tradeSeller, ticker, timeNow);

            string datetime = std::string(ctime(&timeNow));
            trade["datetime"] = datetime.substr(0, datetime.length() - 1); // trim out \n
        }
        // if some amount left untraded, insert to buy_orders
        if (currAmt > 0)
        {
            long long datetime = std::time(nullptr);
            W.exec_prepared("insertBuyOrder", buyer, currAmt, price, ticker, datetime);
        }
        W.commit();
        response = {
            {"placeBuyOrderResponse", trades}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::placeSellOrder(string seller, int price, int amt, string ticker)
{
    json response;
    try
    {
        if (price <= 0 || amt <= 0)
        {
            throw std::invalid_argument( "received negative value for price/amt" );
        }

        if (!validateTicker(ticker))
        {
            throw std::invalid_argument("invalid ticker format");
        }

        int currAmt = amt;
        vector<pair<long long, int>> partiallyConvertedOrders;
        json trades;

        pqxx::work W{C};
        pqxx::result R{W.exec_prepared("getOrdersWithHighestPrice", ticker, price)};
        
        while (R.size() > 0 && currAmt > 0)
        {
            int currPrice = R[0][3].as<int>();
            for (auto row : R)
            {
                long long order_id = row[0].as<long long>();
                string buyer = row[1].as<string>();
                int amt = row[2].as<int>();
                int buyPrice = row[3].as<int>();
                long long datetime = std::time(nullptr);
                if (currAmt >= amt)
                {
                    json trade = {
                        {"price", buyPrice},
                        {"amount", amt},
                        {"buyer", buyer},
                        {"seller", seller},
                        {"ticker", ticker},
                        {"datetime", datetime}
                    };
                    trades.push_back(trade);
                    currAmt -= amt;
                }
                else
                {
                    json trade = {
                        {"price", buyPrice},
                        {"amount", currAmt},
                        {"buyer", buyer},
                        {"seller", seller},
                        {"ticker", ticker},
                        {"datetime", datetime}
                    };
                    partiallyConvertedOrders.push_back(make_pair(order_id, amt - currAmt));
                    trades.push_back(trade);
                    currAmt = 0;
                    break;
                }
            }
            // delete fully converted orders
            if (partiallyConvertedOrders.size() > 0)
            {
                W.exec_prepared("deleteBuyOrdersAtPrice", currPrice, partiallyConvertedOrders[0].first);
            }
            else
            {
                W.exec_prepared("deleteAllBuyOrdersAtPrice", currPrice);
            }
            
            R = {W.exec_prepared("getOrdersWithHighestPrice", ticker, price)};
        }
        // update partially converted order (if any)
        for (int i = 0; i < partiallyConvertedOrders.size(); i++)
        {
            ostringstream updateQuery;
            int amountLeft = partiallyConvertedOrders[i].second;
            long long order_id = partiallyConvertedOrders[i].first;
            
            W.exec_prepared("updatePartialBuyOrder", amountLeft, order_id);
        }
        // insert newly created trades
        for (int i = 0; i < trades.size(); i++)
        {
            json &trade = trades[i];
            
            int tradePrice = trade["price"].get<int>();
            int tradeAmount = trade["amount"].get<int>();
            string tradeBuyer = trade["buyer"].get<string>();
            string tradeSeller = trade["seller"].get<string>();
            long timeNow = trade["datetime"].get<long>();
            W.exec_prepared("insertTrades", tradeAmount, tradePrice, tradeBuyer, tradeSeller, ticker, timeNow);

            string datetime = std::string(ctime(&timeNow));
            trade["datetime"] = datetime.substr(0, datetime.length() - 1); // trim out \n
        }
        // if some amount left untraded, insert to sell_orders
        if (currAmt > 0)
        {
            long long datetime = std::time(nullptr);
            W.exec_prepared("insertSellOrder", seller, currAmt, price, ticker, datetime);
        }
        W.commit();
        response = {
            {"placeSellOrderResponse", trades}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::getPendingBuyOrders(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("getPendingBuyOrders", username)};
        json entries;
        for (auto row : R)
        {
            long long order_id = row[0].as<long long>();
            string username = row[1].as<string>();
            int amt = row[2].as<int>();
            int price = row[3].as<int>();
            string ticker = row[4].as<string>();
            time_t datetimeLong = row[5].as<long long>();
            string datetime = std::string(ctime(&datetimeLong));
            json entry = {
                {"order_id", order_id},
                {"price", price},
                {"amount", amt},
                {"price", price},
                {"ticker", ticker},
                {"datetime", datetime.substr(0, datetime.length() - 1)}
            };
            entries.push_back(entry);
        }
        response = {
            {"getPendingBuyOrdersResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::getPendingSellOrders(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("getPendingSellOrders", username)};
        json entries;
        for (auto row : R)
        {
            long long order_id = row[0].as<long long>();
            string username = row[1].as<string>();
            int amt = row[2].as<int>();
            int price = row[3].as<int>();
            string ticker = row[4].as<string>();
            time_t datetimeLong = row[5].as<long long>();
            string datetime = std::string(ctime(&datetimeLong));
            json entry = {
                {"order_id", order_id},
                {"price", price},
                {"amount", amt},
                {"price", price},
                {"ticker", ticker},
                {"datetime", datetime.substr(0, datetime.length() - 1)}
            };
            entries.push_back(entry);
        }
        response = {
            {"getPendingSellOrdersResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::getBuyTrades(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("getBuyTrades", username)};
        json entries;
        for (auto row : R)
        {
            long long trade_id = row[0].as<long long>();
            int amt = row[1].as<int>();
            int price = row[2].as<int>();
            string buyer = row[3].as<string>();
            string seller = row[4].as<string>();
            string ticker = row[5].as<string>();
            time_t datetimeLong = row[6].as<long long>();
            string datetime = std::string(ctime(&datetimeLong));
            json entry = {
                {"trade_id", trade_id},
                {"price", price},
                {"amount", amt},
                {"buyer", buyer},
                {"seller", seller},
                {"ticker", ticker},
                {"datetime", datetime.substr(0, datetime.length() - 1)}
            };
            entries.push_back(entry);
        }
        response = {
            {"getBuyTradesResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

json TradeEngine::getSellTrades(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("getSellTrades", username)};
        json entries;
        for (auto row : R)
        {
            long long trade_id = row[0].as<long long>();
            int amt = row[1].as<int>();
            int price = row[2].as<int>();
            string buyer = row[3].as<string>();
            string seller = row[4].as<string>();
            string ticker = row[5].as<string>();
            time_t datetimeLong = row[6].as<long long>();
            string datetime = std::string(ctime(&datetimeLong));
            json entry = {
                {"trade_id", trade_id},
                {"price", price},
                {"amount", amt},
                {"buyer", buyer},
                {"seller", seller},
                {"ticker", ticker},
                {"datetime", datetime.substr(0, datetime.length() - 1)}
            };
            entries.push_back(entry);
        }
        response = {
            {"getSellTradesResponse", entries}
        };
    }
    catch (const std::exception &e)
    {
        response = {
            {"errorResponse", {
                {"message", e.what()}
            }}
        };
    }
    return response;
}

void TradeEngine::prepareStatements()
{
    string createUserSQL = "INSERT INTO ts.login (username, password) VALUES ($1,$2)";
    C.prepare("createUser", createUserSQL);

    string getUserPasswordSQL = "SELECT password FROM ts.login WHERE username = $1";
    C.prepare("getUserPassword", getUserPasswordSQL);

    string deleteBuyOrderSQL = "DELETE FROM ts.buy_orders WHERE order_id = $1 AND username = $2 RETURNING *";
    C.prepare("deleteBuyOrder", deleteBuyOrderSQL);

    string deleteSellOrderSQL = "DELETE FROM ts.sell_orders WHERE order_id = $1 AND username = $2";
    C.prepare("deleteSellOrder", deleteSellOrderSQL);

    string getBuyVolumesSQL = "SELECT price, SUM(amount) FROM ts.buy_orders WHERE ticker = $1 GROUP BY price ORDER BY price DESC";
    C.prepare("getBuyVolumes", getBuyVolumesSQL);

    string getSellVolumesSQL = "SELECT price, SUM(amount) FROM ts.sell_orders WHERE ticker = $1 GROUP BY price ORDER BY price ASC";
    C.prepare("getSellVolumes", getSellVolumesSQL);

    string getOrdersWithLowestPriceSQL = "SELECT * FROM ts.sell_orders WHERE ticker = $1 AND price = LEAST((SELECT MIN(price) FROM ts.sell_orders WHERE ticker = $1),  $2) ORDER BY order_id ASC";
    C.prepare("getOrdersWithLowestPrice", getOrdersWithLowestPriceSQL);

    string deleteAllSellOrdersAtPriceSQL = "DELETE FROM ts.sell_orders WHERE price = $1";
    C.prepare("deleteAllSellOrdersAtPrice", deleteAllSellOrdersAtPriceSQL);

    string deleteSellOrdersAtPriceSQL = "DELETE FROM ts.sell_orders WHERE price = $1 AND order_id < $2";
    C.prepare("deleteSellOrdersAtPrice", deleteSellOrdersAtPriceSQL);

    string updatePartialSellOrderSQL = "UPDATE ts.sell_orders SET amount = $1 WHERE order_id = $2";
    C.prepare("updatePartialSellOrder", updatePartialSellOrderSQL);

    string insertBuyOrderSQL = "INSERT INTO ts.buy_orders (username, amount, price, ticker, datetime)  VALUES ($1, $2, $3, $4, $5)";
    C.prepare("insertBuyOrder", insertBuyOrderSQL);

    string getOrdersWithHighestPriceSQL = "SELECT * FROM ts.buy_orders WHERE ticker = $1 AND price = GREATEST((SELECT MAX(price) FROM ts.buy_orders WHERE ticker = $1), $2) ORDER BY order_id ASC";
    C.prepare("getOrdersWithHighestPrice", getOrdersWithHighestPriceSQL);

    string deleteAllBuyOrdersAtPriceSQL = "DELETE FROM ts.buy_orders WHERE price = $1";
    C.prepare("deleteAllBuyOrdersAtPrice", deleteAllBuyOrdersAtPriceSQL);

    string deleteBuyOrdersAtPriceSQL = "DELETE FROM ts.buy_orders WHERE price = $1 AND order_id < $2";
    C.prepare("deleteBuyOrdersAtPrice", deleteBuyOrdersAtPriceSQL);

    string updatePartialBuyOrderSQL = "UPDATE ts.buy_orders SET amount = $1 WHERE order_id = $2";
    C.prepare("updatePartialBuyOrder", updatePartialBuyOrderSQL);

    string insertSellOrderSQL = "INSERT INTO ts.sell_orders (username, amount, price, ticker, datetime) VALUES ($1, $2, $3, $4, $5)";
    C.prepare("insertSellOrder", insertSellOrderSQL);

    string insertTradesSQL = "INSERT INTO ts.trades (amount, price, buyer, seller, ticker, datetime) VALUES ($1, $2, $3, $4, $5, $6)";
    C.prepare("insertTrades", insertTradesSQL);

    string getPendingBuyOrdersSQL = "SELECT * FROM ts.buy_orders WHERE username = $1";
    C.prepare("getPendingBuyOrders", getPendingBuyOrdersSQL);

    string getPendingSellOrdersSQL = "SELECT * FROM ts.sell_orders WHERE username = $1";
    C.prepare("getPendingSellOrders", getPendingSellOrdersSQL); 

    string getBuyTradesSQL = "SELECT * FROM ts.trades WHERE buyer = $1";
    C.prepare("getBuyTrades", getBuyTradesSQL);

    string getSellTradesSQL = "SELECT * FROM ts.trades WHERE seller = $1";
    C.prepare("getSellTrades", getSellTradesSQL);
}

string TradeEngine::hashPassword(const string& password)
{
    return bcrypt::generateHash(password);
}