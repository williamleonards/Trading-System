#include "utils.h"

json formResponse(int id, json resp)
{
    json workerResponse;
    workerResponse["id"] = id;
    workerResponse["response"] = resp;
    return workerResponse;
}

void sendResponse(const json &response, AMQP::Channel &channel,
    const std::string &exchange, const std::string &responseRouting)
{
    if (channel.ready())
    {
        channel.publish(exchange, responseRouting, response.dump() + "\n");
    }
    else
    {
        std::cout << "Can't publish, channel unavailable" << std::endl;
    }
}