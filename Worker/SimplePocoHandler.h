//
// Created by William Leonard Sumendap on 7/20/21.
//

#ifndef SYNCHRONIZER_SIMPLEPOCOHANDLER_H
#define SYNCHRONIZER_SIMPLEPOCOHANDLER_H

#include <memory>
#include <amqpcpp.h>

class SimplePocoHandlerImpl;
class SimplePocoHandler: public AMQP::ConnectionHandler
{
public:

    static constexpr size_t BUFFER_SIZE = 8 * 1024 * 1024; //8Mb
    static constexpr size_t TEMP_BUFFER_SIZE = 1024 * 1024; //1Mb

    SimplePocoHandler(const std::string& host, uint16_t port);
    virtual ~SimplePocoHandler();

    void loop();
    void quit();

    bool connected() const;

private:

    SimplePocoHandler(const SimplePocoHandler&) = delete;
    SimplePocoHandler& operator=(const SimplePocoHandler&) = delete;

    void close();

    virtual void onData(
            AMQP::Connection *connection, const char *data, size_t size);

    virtual void onConnected(AMQP::Connection *connection);

    virtual void onError(AMQP::Connection *connection, const char *message);

    virtual void onClosed(AMQP::Connection *connection);

    void sendDataFromBuffer();

private:

    std::shared_ptr<SimplePocoHandlerImpl> m_impl;
};

#endif //SYNCHRONIZER_SIMPLEPOCOHANDLER_H
