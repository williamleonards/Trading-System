#include <iostream>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <memory>
#include <fstream>
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

bool loginSuccess(json result)
{
    return result["loginUserResponse"].get<bool>();
}

void sendResponse(HTTPServerResponse& response, json &result)
{
    response.send() << result.dump() << "\n";
}

void sendSessionNotFoundResponse(HTTPServerResponse& response)
{
    json result;
    result["error"] = "Session token or username not found";
    sendResponse(response, result);
}

void sendMissingHeaderResponse(HTTPServerResponse& response, NotFoundException &e)
{
    json result;
    result["error"] = "Argument " + e.message() + " not found";
    sendResponse(response, result);
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

        json result;
        result["helloResponse"] = "Hello from the TS Server!";

        sendResponse(response, result);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "register";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send() << result.dump()  << "\n";
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
            sendMissingHeaderResponse(response, e);
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

        sendResponse(response, result);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        bool status = sessionService.removeSession(username);

        json result;
        result["logoutUserResponse"] = status;

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "delete-buy";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "delete-sell";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "buy";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "sell";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "buy-tree";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "sell-tree";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            std::cout << "session not found..." << std::endl;
            sendSessionNotFoundResponse(response);
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
            std::cout << "header not found..." << std::endl;
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "pending-buy";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "pending-sell";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        args["method"] = "buy-history";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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
            sendMissingHeaderResponse(response, e);
            return;
        }

        // TODO: ADD EXCEPTION HANDLING FOR PARSE EXCEPTIONS THROWN BY WORKER

        args["method"] = "sell-history";
        json result = sync.query(args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        sendResponse(response, result);
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
            sendSessionNotFoundResponse(response);
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

        sendResponse(response, result);
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
        // ROUTER CLASS!!!
        // SAFE FROM MEMORY LEAK AS THE CALLER WILL CLEANUP
        // (SEE POCO EXAMPLES https://pocoproject.org/slides/200-Network.pdf)
        std::string path = request.getURI();
        if (path == "/")
        {
            return new HelloRequestHandler(sync);
        }
        else if (path == "/register")
        {
            return new RegisterRequestHandler(sync);
        }
        else if (path == "/login")
        {
            return new LoginRequestHandler(sync, sessionService);
        }
        else if (path == "/logout")
        {
            return new LogoutRequestHandler(sessionService);
        }
        else if (path == "/delete-buy")
        {
            return new DeleteBuyOrderRequestHandler(sync, sessionService);
        }
        else if (path == "/delete-sell")
        {
            return new DeleteSellOrderRequestHandler(sync, sessionService);
        }
        else if (path == "/buy")
        {
            return new BuyOrderRequestHandler(sync, sessionService);
        }
        else if (path == "/sell")
        {
            return new SellOrderRequestHandler(sync, sessionService);
        }
        else if (path == "/buy-tree")
        {
            return new ViewBuyTreeRequestHandler(sync, sessionService);
        }
        else if (path == "/sell-tree")
        {
            return new ViewSellTreeRequestHandler(sync, sessionService);
        }
        else if (path == "/pending-buy")
        {
            return new ViewPendingBuyOrderRequestHandler(sync, sessionService);
        }
        else if (path == "/pending-sell")
        {
            return new ViewPendingSellOrderRequestHandler(sync, sessionService);
        }
        else if (path == "/buy-history")
        {
            return new ViewBuyHistoryRequestHandler(sync, sessionService);
        }
        else if (path == "/sell-history")
        {
            return new ViewSellHistoryRequestHandler(sync, sessionService);
        }
        else {
            return new UnknownRequestHandler(sync, sessionService);
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
        // READ CONFIG JSON
        std::ifstream file("../dispatcher-config.json"); // PATH RELATIVE TO EXECUTABLE, NOT SOURCE CODE
        std::stringstream buffer;
        buffer << file.rdbuf();
        json dispatcherConfig = json::parse(buffer.str());
        
        Synchronizer sync(dispatcherConfig["synchronizerConfig"]);
        sync.start();

        SessionManager sessionService(dispatcherConfig["sessionServiceConfig"]);

        DispatcherRequestHandlerFactory dispatcher(sync, sessionService); 
        UInt16 port = static_cast<UInt16>(config().getUInt("port", dispatcherConfig["dispatcherServerConfig"]["port"].get<int>()));

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