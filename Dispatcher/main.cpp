#include <iostream>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <stdlib.h>
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

#include <nlohmann/json.hpp>

using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;

using json = nlohmann::json;

int HALF_HOUR = 1800000;

std::string extractParams(HTTPServerRequest& request, std::vector<std::string> headers)
{
    std::string args = "";
    for (int i = 0; i < headers.size(); i++)
    {
        std::string header = headers[i];
        const std::string& arg = request.get(header);
        args += arg;
        args += "|";
    }
    return args;
}

bool checkSessionToken(HTTPServerRequest& request, SessionManager &sessionService)
{
    try
    {
        const std::string &username = request.get("username");
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
    // result.find("true") != std::string::npos
    std::cout << "result is ... " << result << std::endl;
    return true;
}

class HelloRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("hello|");

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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username", "password"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("register|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Registration complete with ID " << result  << "\n";
    }
public:
    RegisterRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

// TODO: COMPLETE LOGIN SERVICE AND CALL USING THIS FUNCTION
class LoginRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    SessionManager &sessionService;
    
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username", "password"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("login|" + args);
        
        if (loginSuccess(result))
        {
            std::string username = request.get("username");
            HTTPCookie cookie;
            sessionService.registerSession(username, cookie);
            response.addCookie(cookie);
        }
        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Login executed with " << result  << "\n";
    }
public:
    LoginRequestHandler(Synchronizer &sync_, SessionManager &sessionService_):
    sync(sync_),
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username", "orderId"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("delete-buy|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << result << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username", "orderId"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception on field " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("delete-sell|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << result << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username", "price", "amount", "ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("buy|" + args);
        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Place buy order successful, result is " << result  << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username", "price", "amount", "ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("sell|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Place sell order successful, result is " << result  << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("buy-tree|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View buy tree successful, result is " << result  << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"ticker"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("sell-tree|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View sell tree successful, result is " << result  << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("pending-buy|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View pending buy order successful, result is " << result  << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("pending-sell|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View pending buy order successful, result is " << result  << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        std::string result = sync.query("buy-history|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View buy history successful, result is " << result  << "\n";
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

        std::string args;
        try
        {
            args = extractParams(request, std::vector<std::string>{"username"});
        }
        catch (NotFoundException e)
        {
            response.send()
                    << "Argument parse error with exception " << e.message() << "\n";
            return;
        }

        // TODO: ADD EXCEPTION HANDLING FOR PARSE EXCEPTIONS THROWN BY WORKER
        std::string result = sync.query("sell-history|" + args);

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View sell history successful, result is " << result  << "\n";
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

        std::string result = sync.query("unknown-request|");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Unknown request, response from server: " << result  << "\n";
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
        // MEMORY LEAK!!!!!
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