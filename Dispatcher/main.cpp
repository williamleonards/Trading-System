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

void *sendQuery(void *arg)
{
    State *s = (State *) arg;
    Synchronizer *sync = s->sync;
    int n = s->n;
    int connection = s->connection;

    // Read from the connection
    char buffer[100];
    auto bytesRead = read(connection, buffer, 100);
    std::cout << "The message was: " << buffer;

    int ans = sync->query(n);

    // Send a message to the connection
    std::string response = "The answer was " + std::to_string(ans);
    std::cout << response << std::endl;
    send(connection, response.c_str(), response.size(), 0);
    close(connection);
}

class HelloRequestHandler: public HTTPRequestHandler
{
    Synchronizer &sync;
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
    {
        Application& app = Application::instance();
        app.logger().information("Request from %s", request.clientAddress().toString());
        std::cout << request.getURI() << std::endl;

        int result = sync.query(10);

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

        int result = sync.query(20);

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

        int result = sync.query(30);

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

        int result = sync.query(40);

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

        int result = sync.query(50);

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

        int result = sync.query(60);

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

        int result = sync.query(70);

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

// TODO: REMOVE PREVIOUS MAIN METHOD USING SOCKETS
int oldMain()
{
    Synchronizer *sync = new Synchronizer(20);
    pthread_t starter;
    pthread_create(&starter, NULL, &startSync, (void *) sync);

    // Create a socket (IPv4, TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cout << "Failed to create socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Listen to port 9999 on any address
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(9999); // htons is necessary to convert a number to
    // network byte order
    if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        std::cout << "Failed to bind to port 9999. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Start listening. Hold at most 10 connections in the queue
    if (listen(sockfd, 10) < 0) {
        std::cout << "Failed to listen on socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        // Grab a connection from the queue
        auto addrlen = sizeof(sockaddr);
        int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
        if (connection < 0) {
            std::cout << "Failed to grab connection. errno: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }

        // SPAWN A NEW THREAD RESPONSIBLE FOR QUERYING
        State *st = new State(sync, rand() % 1000, connection);
        pthread_t queryThread;
        pthread_create(&queryThread, NULL, &sendQuery, (void *) st);
    }


    // Close the connections
    close(sockfd);

    pthread_join(starter, NULL);

    return 0;
}