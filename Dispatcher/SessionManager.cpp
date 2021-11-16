#include "SessionManager.h"
#include <string>
#include <iostream>

SessionManager::SessionManager(std::string connection, std::chrono::milliseconds timeout)
    : service(connection), timeout(timeout)
{
}

bool SessionManager::registerSession(std::string &username, HTTPCookie &resultCookie)
{
    HTTPCookie cookie(username);
    cookie.setValue(generateSessionToken(username));
    resultCookie = cookie;
    std::string token = cookie.toString();
    bool res = service.set(username, token, timeout);
    return res;
}

bool SessionManager::check(const std::string &username, const std::string &token)
{
    auto val = service.get(username);
    if (!val) return false;
    return *val == token;
}

// TODO: IMPLEMENT THIS
std::string SessionManager::generateSessionToken(std::string &username)
{
    return "ABCABCABC";
}