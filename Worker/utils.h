//
// Created by William Leonard Sumendap on 12/27/21.
//

#ifndef WORKER_UTILS_H
#define WORKER_UTILS_H

#include <amqpcpp.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json formResponse(int id, json resp);

void sendResponse(const json &response, AMQP::Channel &channel,
    const std::string &exchange, const std::string &responseRouting);

#endif // WORKER_UTILS_H