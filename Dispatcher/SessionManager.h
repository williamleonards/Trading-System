//
// Created by William Leonard Sumendap on 11/11/21.
//

#ifndef DISPATCHER_SESSION_MANAGER_H
#define DISPATCHER_SESSION_MANAGER_H

#include <sw/redis++/redis++.h>
#include "Poco/Net/HTTPCookie.h"
#include <nlohmann/json.hpp>

using namespace sw::redis;
using namespace Poco::Net;
using json = nlohmann::json;

class SessionManager
{
public:
    SessionManager(json config);
    bool registerSession(std::string &username, HTTPCookie &cookie);
    bool removeSession(std::string &username);
    bool check(const std::string &username, const std::string &token);

private:
    std::string generateSessionToken(std::string &username);

private:
    Redis service;
    std::chrono::milliseconds timeout;
};
#endif //DISPATCHER_SESSION_MANAGER_H
