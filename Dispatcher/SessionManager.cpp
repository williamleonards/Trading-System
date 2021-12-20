#include "SessionManager.h"
#include <string>
#include <iostream>

SessionManager::SessionManager(json config):
    service(config["redisConnection"].get<std::string>()),
    timeout(std::chrono::milliseconds(config["timeout"].get<int>())),
    timeoutInSeconds(config["timeout"].get<int>() / 1000)
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

bool SessionManager::removeSession(std::string &username)
{
    return service.del(username);
}

bool SessionManager::check(const std::string &username, const std::string &token)
{
    auto val = service.get(username);
    if (!val) return false;
    // ALSO REFRESH TOKEN EXPIRATION
    service.expire(username, timeoutInSeconds);
    return *val == token;
}

// TODO: IMPLEMENT THIS
std::string SessionManager::generateSessionToken(std::string &username)
{
    return "ABCABCABC";
}