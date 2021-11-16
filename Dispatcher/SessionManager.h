//
// Created by William Leonard Sumendap on 11/11/21.
//

#ifndef DISPATCHER_SESSION_MANAGER_H
#define DISPATCHER_SESSION_MANAGER_H

#include <sw/redis++/redis++.h>
#include "Poco/Net/HTTPCookie.h"

using namespace sw::redis;
using namespace Poco::Net;

class SessionManager
{
public:
    SessionManager(std::string connection, std::chrono::milliseconds timeout);
    bool registerSession(std::string &username, HTTPCookie &cookie);
    bool check(const std::string &username, const std::string &token);

private:
    std::string generateSessionToken(std::string &username);

private:
    Redis service;
    std::chrono::milliseconds timeout;
};
#endif //DISPATCHER_SESSION_MANAGER_H
