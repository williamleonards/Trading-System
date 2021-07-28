#include <iostream>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include "Synchronizer.h"

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

void *startSync(void *arg)
{
    Synchronizer *sync = (Synchronizer *) arg;

    sync->start();
}

struct State
{
    Synchronizer *sync;
    int n;
    int connection;
    State(Synchronizer* sync_, int n_, int connection_): n(n_), connection(connection_)
    {
        sync = sync_;
    }
};

class HelloRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("10");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Hello from the TS Server, result is " << result  << "\n";
    }
public:
    HelloRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class DeleteOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("20");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Delete successful, result is " << result  << "\n";
    }
public:
    DeleteOrderRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class PlaceOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("30");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Place order successful, result is " << result  << "\n";
    }
public:
    PlaceOrderRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class ViewTreeRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("40");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View tree successful, result is " << result  << "\n";
    }
public:
    ViewTreeRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class ViewPendingOrderRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("50");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View pending order successful, result is " << result  << "\n";
    }
public:
    ViewPendingOrderRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class ViewOrderHistoryRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("60");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "View order history successful, result is " << result  << "\n";
    }
public:
    ViewOrderHistoryRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};
class UnknownRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        std::string result = sync.query("70");

        response.setChunkedTransferEncoding(true);
        response.setContentType("text/html");

        response.send()
                << "Unknown request, result is " << result  << "\n";
    }
public:
    UnknownRequestHandler(Synchronizer &sync_) : sync(sync_) {}
};

class DispatcherRequestHandlerFactory: public HTTPRequestHandlerFactory
{
    Synchronizer &sync;
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
    {
        std::string path = request.getURI();
        if (path == "/")
        {
            return new HelloRequestHandler(sync);
        }
        else if (path == "/delete")
        {
            return new DeleteOrderRequestHandler(sync);
        }
        else if (path == "/place")
        {
            return new PlaceOrderRequestHandler(sync);
        }
        else if (path == "/tree")
        {
            return new ViewTreeRequestHandler(sync);
        }
        else if (path == "/pending")
        {
            return new ViewPendingOrderRequestHandler(sync);
        }
        else if (path == "/history")
        {
            return new ViewOrderHistoryRequestHandler(sync);
        }
        else {
            return new UnknownRequestHandler(sync);
        }
    }
public:
    DispatcherRequestHandlerFactory(Synchronizer &sync_) : sync(sync_) {}
};

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

        UInt16 port = static_cast<UInt16>(config().getUInt("port", 8080));

        HTTPServer srv(new DispatcherRequestHandlerFactory(sync), port);
        srv.start();
        logger().information("HTTP Server started on port %hu.", port);
        waitForTerminationRequest();
        logger().information("Stopping HTTP Server...");
        srv.stop();

        return Application::EXIT_OK;
    }
};

POCO_SERVER_MAIN(WebServerApp)