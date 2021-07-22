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

void *startSync(void *arg) {

    Synchronizer *sync = (Synchronizer *) arg;

    sync->start();
}

struct State {
    Synchronizer *sync;
    int n;
    int connection;
    State(Synchronizer* sync_, int n_, int connection_): n(n_), connection(connection_)
    {
        sync = sync_;
    }
};

void *sendQuery(void *arg) {

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

int main() {
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