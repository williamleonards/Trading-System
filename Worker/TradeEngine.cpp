//
// Created by William Leonard Sumendap on 7/10/20.
//

#include "TradeEngine.h"
#include <pthread.h>

TradeEngine::TradeEngine(string conn) : C(conn)
{
    prepareStatements();
}

string TradeEngine::createUser(string name, string password)
{
    pqxx::work W{C};
    json response;
    try
    {
        W.exec_prepared("createUser", name, password);
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
    return response.dump();
}

string TradeEngine::deleteBuyOrder(string username, long long orderId)
{
    //lazy deletion (?)
    pqxx::work W{C};
    json response;
    try
    {
        W.exec_prepared("deleteBuyOrder", orderId, username);
        W.commit();
        response = {
            {"deleteBuyOrderResponse", {}}
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
    return response.dump();
}

string TradeEngine::deleteSellOrder(string username, long long orderId)
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
    return response.dump();
}

string TradeEngine::getBuyVolumes()
{
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("getBuyVolumes")};
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
            {"getBuyVolumesResponse", entries}
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
    return response.dump();
}

string TradeEngine::getSellVolumes()
{
    pqxx::work W{C};
    json response;
    try
    {
        pqxx::result R{W.exec_prepared("getSellVolumes")};
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
            {"getSellVolumesResponse", entries}
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
    return response.dump();
}

string TradeEngine::placeBuyOrder(string buyer, int price, int amt)
{
    json response;
    try
    {
        int currAmt = amt;
        vector<pair<long long, int>> partiallyConvertedOrders;
        json trades;
        // ostringstream query;
        // query << "SELECT * FROM ts.sell_orders WHERE price = LEAST((SELECT MIN(price) "
        //     << "FROM ts.sell_orders), " << price << ") ORDER BY order_id ASC";

        // pqxx::result R{W.exec(query.str())};
        
        pqxx::work W{C};
        pqxx::result R{W.exec_prepared("getOrdersWithLowestPrice", price)};
        
        while (R.size() > 0 && currAmt > 0)
        {
            int currPrice = R[0][3].as<int>();
            for (auto row : R)
            {
                long long order_id = row[0].as<long long>();
                string seller = row[1].as<string>();
                int amt = row[2].as<int>();
                int sellPrice = row[3].as<int>();
                if (currAmt >= amt)
                {
                    json trade = {
                        {"price", sellPrice},
                        {"amount", amt},
                        {"buyer", buyer},
                        {"seller", seller}
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
                        {"seller", seller}
                    };
                    partiallyConvertedOrders.push_back(make_pair(order_id, amt - currAmt));
                    trades.push_back(trade);
                    currAmt = 0;
                    break; // BREAK WHICH LOOP?
                }
            }
            // delete fully converted orders
            // ostringstream deleteQuery;
            // deleteQuery << "DELETE FROM ts.sell_orders WHERE price = " << currPrice;
            if (partiallyConvertedOrders.size() > 0)
            {
                // deleteQuery << " AND order_id < " << partiallyConvertedOrders[0].first;
                W.exec_prepared("deleteSellOrdersAtPrice", currPrice, partiallyConvertedOrders[0].first);
            }
            else
            {
                W.exec_prepared("deleteAllSellOrdersAtPrice", currPrice);
            }
            // W.exec(deleteQuery.str());
            // R = {W.exec(query.str())}; // CONFIRM HOW TO DO THIS
            R = {W.exec_prepared("getOrdersWithLowestPrice", price)};
        }
        // update partially converted order (if any)
        for (int i = 0; i < partiallyConvertedOrders.size(); i++)
        {
            ostringstream updateQuery;
            int amountLeft = partiallyConvertedOrders[i].second;
            long long order_id = partiallyConvertedOrders[i].first;
            // updateQuery << "UPDATE ts.sell_orders SET amount = " << amountLeft
            //     << " WHERE order_id = " << order_id;
            // W.exec(updateQuery.str());
            W.exec_prepared("updatePartialSellOrder", amountLeft, order_id);
        }
        // insert newly created trades
        for (int i = 0; i < trades.size(); i++)
        {
            json &trade = trades[i];
            // CONFIRM THE SYNTAX
            int tradePrice = trade["price"].get<int>();
            int tradeAmount = trade["amount"].get<int>();
            string tradeBuyer = trade["buyer"].get<string>();
            string tradeSeller = trade["seller"].get<string>();

            // ostringstream insertTradeQuery;
            // insertTradeQuery << "INSERT INTO ts.trades (amount, price, buyer, seller)" 
            //     << " VALUES (" << tradeAmount << ", "
            //     << tradePrice << ", " << tradeBuyer << ", " << tradeSeller << ")";
            // W.exec(insertTradeQuery.str());

            W.exec_prepared("insertTrades", tradeAmount, tradePrice, tradeBuyer, tradeSeller);
        }
        // if some amount left untraded, insert to buy_orders
        if (currAmt > 0)
        {
            // ostringstream insertOrderQuery;
            // insertOrderQuery << "INSERT INTO ts.buy_orders (username, amount, price) " 
            //     << " VALUES (" << buyer << ", " 
            //     << currAmt << ", " << price << ", true)";
            // W.exec(insertOrderQuery.str());
            W.exec_prepared("insertBuyOrder", buyer, currAmt, price);
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
    return response.dump();
}

string TradeEngine::placeSellOrder(string seller, int price, int amt)
{
    json response;
    try
    {
        int currAmt = amt;
        vector<pair<long long, int>> partiallyConvertedOrders;
        json trades;

        // ostringstream query;
        // query << "SELECT * FROM ts.buy_orders WHERE price = GREATEST((SELECT MAX(price) "
        //     << "FROM ts.buy_orders), " << price << ") ORDER BY order_id ASC";

        // pqxx::result R{W.exec(query.str())};

        pqxx::work W{C};
        pqxx::result R{W.exec_prepared("getOrdersWithHighestPrice", price)};
        
        while (R.size() > 0 && currAmt > 0)
        {
            int currPrice = R[0][3].as<int>();
            for (auto row : R)
            {
                long long order_id = row[0].as<long long>();
                string buyer = row[1].as<string>();
                int amt = row[2].as<int>();
                int buyPrice = row[3].as<int>();
                if (currAmt >= amt)
                {
                    json trade = {
                        {"price", buyPrice},
                        {"amount", amt},
                        {"buyer", buyer},
                        {"seller", seller}
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
                        {"seller", seller}
                    };
                    partiallyConvertedOrders.push_back(make_pair(order_id, amt - currAmt));
                    trades.push_back(trade);
                    currAmt = 0;
                    break; // BREAK WHICH LOOP?
                }
            }
            // delete fully converted orders
            ostringstream deleteQuery;
            deleteQuery << "DELETE FROM ts.buy_orders WHERE price = " << currPrice;
            if (partiallyConvertedOrders.size() > 0)
            {
                // deleteQuery << " AND order_id < " << partiallyConvertedOrders[0].first;
                W.exec_prepared("deleteBuyOrdersAtPrice", currPrice, partiallyConvertedOrders[0].first);
            }
            else
            {
                W.exec_prepared("deleteAllBuyOrdersAtPrice", currPrice);
            }
            // W.exec(deleteQuery.str());
            // R = {W.exec(query.str())}; // CONFIRM HOW TO DO THIS
            R = {W.exec_prepared("getOrdersWithHighestPrice", price)};
        }
        // update partially converted order (if any)
        for (int i = 0; i < partiallyConvertedOrders.size(); i++)
        {
            ostringstream updateQuery;
            int amountLeft = partiallyConvertedOrders[i].second;
            long long order_id = partiallyConvertedOrders[i].first;
            // updateQuery << "UPDATE ts.buy_orders SET amount = " << amountLeft
            //     << " WHERE order_id = " << order_id;
            // W.exec(updateQuery.str());
            W.exec_prepared("updatePartialBuyOrder", amountLeft, order_id);
        }
        // insert newly created trades
        for (int i = 0; i < trades.size(); i++)
        {
            json &trade = trades[i];
            // CONFIRM THE SYNTAX
            int tradePrice = trade["price"].get<int>();
            int tradeAmount = trade["amount"].get<int>();
            string tradeBuyer = trade["buyer"].get<string>();
            string tradeSeller = trade["seller"].get<string>();

            // ostringstream insertTradeQuery;
            // insertTradeQuery << "INSERT INTO ts.trades (amount, price, buyer, seller)" 
            //     << " VALUES (" << tradeAmount << ", "
            //     << tradePrice << ", " << tradeBuyer << ", " << tradeSeller << ")";
            // W.exec(insertTradeQuery.str());

            W.exec_prepared("insertTrades", tradeAmount, tradePrice, tradeBuyer, tradeSeller);
        }
        // if some amount left untraded, insert to buy_orders
        if (currAmt > 0)
        {
            // ostringstream insertOrderQuery;
            // insertOrderQuery << "INSERT INTO ts.sell_orders (username, amount, price)" 
            //     << " VALUES (" << seller << ", "
            //     << currAmt << ", " << price << ", true)";
            // W.exec(insertOrderQuery.str());
            W.exec_prepared("insertSellOrder", seller, currAmt, price);
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
    return response.dump();
}

string TradeEngine::getPendingBuyOrders(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.buy_orders WHERE username = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long order_id = row[0].as<long long>();
            string username = row[1].as<string>();
            int amt = row[2].as<int>();
            int price = row[3].as<int>();
            json entry = {
                {"order_id", order_id},
                {"price", price},
                {"amount", amt},
                {"price", price}
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
    return response.dump();
}

string TradeEngine::getPendingSellOrders(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.sell_orders WHERE username = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long order_id = row[0].as<long long>();
            string username = row[1].as<string>();
            int amt = row[2].as<int>();
            int price = row[3].as<int>();
            json entry = {
                {"order_id", order_id},
                {"price", price},
                {"amount", amt},
                {"price", price}
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
    return response.dump();
}

string TradeEngine::getBuyTrades(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.trades WHERE buyer = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long trade_id = row[0].as<long long>();
            int amt = row[1].as<int>();
            int price = row[2].as<int>();
            string buyer = row[3].as<string>();
            string seller = row[4].as<string>();
            json entry = {
                {"trade_id", trade_id},
                {"price", price},
                {"amount", amt},
                {"buyer", buyer},
                {"seller", seller}
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
    return response.dump();
}

string TradeEngine::getSellTrades(string username)
{
    pqxx::work W{C};
    json response;
    try
    {
        string query = "SELECT * FROM ts.trades WHERE seller = '" +
                    username + "'";
        pqxx::result R{W.exec(query)};
        json entries;
        for (auto row : R)
        {
            long long trade_id = row[0].as<long long>();
            int amt = row[1].as<int>();
            int price = row[2].as<int>();
            string buyer = row[3].as<string>();
            string seller = row[4].as<string>();
            json entry = {
                {"trade_id", trade_id},
                {"price", price},
                {"amount", amt},
                {"buyer", buyer},
                {"seller", seller}
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
    return response.dump();
}

void TradeEngine::prepareStatements()
{
    string createUserSQL = "INSERT INTO ts.login (username, password) VALUES ($1,$2)";
    C.prepare("createUser", createUserSQL);

    string deleteBuyOrderSQL = "DELETE FROM ts.buy_orders WHERE order_id = $1 AND username = $2";
    C.prepare("deleteBuyOrder", deleteBuyOrderSQL);

    string deleteSellOrderSQL = "DELETE FROM ts.sell_orders WHERE order_id = $1 AND username = $2";
    C.prepare("deleteSellOrder", deleteSellOrderSQL);

    string getBuyVolumesSQL = "SELECT price, SUM(amount) FROM ts.buy_orders GROUP BY price ORDER BY price DESC";
    C.prepare("getBuyVolumes", getBuyVolumesSQL);

    string getSellVolumesSQL = "SELECT price, SUM(amount) FROM ts.sell_orders GROUP BY price ORDER BY price ASC";
    C.prepare("getSellVolumes", getSellVolumesSQL);

    string getOrdersWithLowestPriceSQL = "SELECT * FROM ts.sell_orders WHERE price = LEAST((SELECT MIN(price) FROM ts.sell_orders),  $1) ORDER BY order_id ASC";
    C.prepare("getOrdersWithLowestPrice", getOrdersWithLowestPriceSQL);

    string deleteAllSellOrdersAtPriceSQL = "DELETE FROM ts.sell_orders WHERE price = $1";
    C.prepare("deleteAllSellOrdersAtPrice", deleteAllSellOrdersAtPriceSQL);

    string deleteSellOrdersAtPriceSQL = "DELETE FROM ts.sell_orders WHERE price = $1 AND order_id < $2";
    C.prepare("deleteSellOrdersAtPrice", deleteSellOrdersAtPriceSQL);

    string updatePartialSellOrderSQL = "UPDATE ts.sell_orders SET amount = $1 WHERE order_id = $2";
    C.prepare("updatePartialSellOrder", updatePartialSellOrderSQL);

    string insertBuyOrderSQL = "INSERT INTO ts.buy_orders (username, amount, price, valid_bit)  VALUES ($1, $2, $3, true)";
    C.prepare("insertBuyOrder", insertBuyOrderSQL);

    string getOrdersWithHighestPriceSQL = "SELECT * FROM ts.buy_orders WHERE price = GREATEST((SELECT MAX(price) FROM ts.buy_orders), $1) ORDER BY order_id ASC";
    C.prepare("getOrdersWithHighestPrice", getOrdersWithHighestPriceSQL);

    string deleteAllBuyOrdersAtPriceSQL = "DELETE FROM ts.buy_orders WHERE price = $1";
    C.prepare("deleteAllBuyOrdersAtPrice", deleteAllBuyOrdersAtPriceSQL);

    string deleteBuyOrdersAtPriceSQL = "DELETE FROM ts.buy_orders WHERE price = $1 AND order_id < $2";
    C.prepare("deleteBuyOrdersAtPrice", deleteBuyOrdersAtPriceSQL);

    string updatePartialBuyOrderSQL = "UPDATE ts.buy_orders SET amount = $1 WHERE order_id = $2";
    C.prepare("updatePartialBuyOrder", updatePartialBuyOrderSQL);

    string insertSellOrderSQL = "INSERT INTO ts.sell_orders (username, amount, price, valid_bit) VALUES ($1, $2, $3, true)";
    C.prepare("insertSellOrder", insertSellOrderSQL);

    string insertTradesSQL = "INSERT INTO ts.trades (amount, price, buyer, seller) VALUES ($1, $2, $3, $4)";
    C.prepare("insertTrades", insertTradesSQL);

}