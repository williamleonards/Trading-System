#include <iostream>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <memory>
#include "Synchronizer.h"
#include "SessionManager.h"

#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <unistd.h> // For read

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Util/ServerApplication.h"

using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;

using json = nlohmann::json;

const int HALF_HOUR = 1800000;

void extractParams(json &args, HTTPServerRequest& request, std::vector<std::string> headers)
{
    for (int i = 0; i < headers.size(); i++)
    {
        std::string header = headers[i];
        const std::string& arg = request.get(header);
        args[header] = arg;
    }
}

bool checkSessionToken(HTTPServerRequest& request, SessionManager &sessionService)
{
    try
    {
        const std::string &username = request.get("username");
        // TODO: THIS BREAKS WHEN THERE ARE MULTIPLE K-V PAIRS
        const std::string &token = request.get("Cookie");
        return sessionService.check(username, token);
    }
    catch (std::exception &e)
    {
        return false;
    }
}

// TODO: IMPLEMENT THIS
bool loginSuccess(std::string result)
{
    return result.find("true") != std::string::npos;
}

class HelloRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Hello from the TS Server!\n";
    }
public:
    HelloRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class RegisterRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username", "password"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        args["method"] = "register";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Registration complete with ID " << result.dump()  << "\n";
    }
public:
    RegisterRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class LoginRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;
    
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username", "password"});
        }
        catch (NotFoundException e)
        {
            response.setChunkedTransferEncoding(true);
            response.setContentType("text/html");
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        args["method"] = "login";
        json result = sync.query(args);
        
        if (loginSuccess(result))
        {
            std::string username = request.get("username"); // guaranteed to succeed
            HTTPCookie cookie;
            sessionService.registerSession(username, cookie);
            response.addCookie(cookie);
        }
        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Login executed with result " << result.dump()  << "\n";
    }
public:
    LoginRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class LogoutRequestHandler: public HTTPRequestHandler
{
    SessionManager &sessionService;
    
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;
        
        std::string username;
        try
        {
            username = request.get("username");
        }
        catch (NotFoundException e)
        {
            response.setChunkedTransferEncoding(true);
            response.setContentType("text/html");
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        std::string result = sessionService.removeSession(username) ? "true" : "false";
        std::string resp = "{\"logoutUserResponse\": " + result + "}";

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Logout executed with result " << resp << "\n";
    }
public:
    LogoutRequestHandler(SessionManager &sessionService_):
    sessionService(sessionService_)
    {}
};

class DeleteBuyOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username", "orderId"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        args["method"] = "delete-buy";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Delete buy executed with result " << result.dump() << "\n";
    }
public:
    DeleteBuyOrderRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class DeleteSellOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username", "orderId"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        args["method"] = "delete-sell";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Delete sell executed with result " << result.dump() << "\n";
    }
public:
    DeleteSellOrderRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class BuyOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username", "price", "amount", "ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        args["method"] = "buy";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Place buy order successful, result is " << result.dump()  << "\n";
    }
public:
    BuyOrderRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class SellOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username", "price", "amount", "ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        args["method"] = "sell";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Place sell order successful, result is " << result.dump()  << "\n";
    }
public:
    SellOrderRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class ViewBuyTreeRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        args["method"] = "buy-tree";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View buy tree successful, result is " << result.dump()  << "\n";
    }
public:
    ViewBuyTreeRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class ViewSellTreeRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        args["method"] = "sell-tree";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View sell tree successful, result is " << result.dump()  << "\n";
    }
public:
    ViewSellTreeRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class ViewPendingBuyOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        args["method"] = "pending-buy";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View pending buy order successful, result is " << result.dump()  << "\n";
    }
public:
    ViewPendingBuyOrderRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class ViewPendingSellOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        args["method"] = "pending-sell";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View pending buy order successful, result is " << result.dump()  << "\n";
    }
public:
    ViewPendingSellOrderRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class ViewBuyHistoryRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;

        try
        {
            extractParams(args, request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        args["method"] = "buy-history";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View buy history successful, result is " << result.dump()  << "\n";
    }
public:
    ViewBuyHistoryRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class ViewSellHistoryRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;
        try
        {
            extractParams(args, request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        // TODO: ADD EXCEPTION HANDLING FOR PARSE EXCEPTIONS THROWN BY WORKER

        args["method"] = "sell-history";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View sell history successful, result is " << result.dump()  << "\n";
    }
public:
    ViewSellHistoryRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class UnknownRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;

    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        if (!checkSessionToken(request, sessionService))
        {
            response.send()
                    << "Session token or username not found" << "\n";
            return;
        }
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        json args;
        args["method"] = "unknown-request";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Unknown request, response from server: " << result.dump()  << "\n";
    }
public:
    UnknownRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

class DispatcherRequestHandlerFactory: public HTTPRequestHandlerFactory
{
    Synchronizer &sync;
    SessionManager &sessionService;
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
    {
        // router class
        // MEMORY LEAK!!!!! USE SMART PTRS!!!!
        std::string path = request.getURI();
        if (path == "/")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new HelloRequestHandler(sync));
            return ptr.get();
        }
        else if (path == "/register")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new RegisterRequestHandler(sync));
            return ptr.get();
        }
        else if (path == "/login")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new LoginRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/logout")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new LogoutRequestHandler(sessionService));
            return ptr.get();
        }
        else if (path == "/delete-buy")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new DeleteBuyOrderRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/delete-sell")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new DeleteSellOrderRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/buy")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new BuyOrderRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/sell")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new SellOrderRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/buy-tree")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new ViewBuyTreeRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/sell-tree")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new ViewSellTreeRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/pending-buy")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new ViewPendingBuyOrderRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/pending-sell")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new ViewPendingSellOrderRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/buy-history")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new ViewBuyHistoryRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else if (path == "/sell-history")
        {
            std::shared_ptr<HTTPRequestHandler> ptr(new ViewSellHistoryRequestHandler(sync, sessionService));
            return ptr.get();
        }
        else {
            std::shared_ptr<HTTPRequestHandler> ptr(new UnknownRequestHandler(sync, sessionService));
            return ptr.get();
        }
    }
public:
    DispatcherRequestHandlerFactory(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
    sessionService(sessionService_)
    {}
};

// ONLY THIS REMAINS ON MAIN
class WebServerApp: public ServerApplication
{
    void initialize(Application& self)
    {
        loadConfiguration();
        ServerApplication::initialize(self);
    }

    int main(const std::vector<std::string>&)
    {
        Synchronizer sync(10);
        sync.start();

        SessionManager sessionService("tcp://127.0.0.1:6379", std::chrono::milliseconds(HALF_HOUR));

        DispatcherRequestHandlerFactory dispatcher(sync, sessionService); 
        UInt16 port = static_cast<UInt16>(config().getUInt("port", 8080));

        HTTPServer srv(&dispatcher, port);
        srv.start();
        logger().information("HTTP Server started on port %hu.", port);
        waitForTerminationRequest();
        logger().information("Stopping HTTP Server...");
        srv.stop();

        return Application::EXIT_OK;
    }
};

POCO_SERVER_MAIN(WebServerApp)