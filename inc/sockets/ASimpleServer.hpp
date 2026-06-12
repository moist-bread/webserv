#pragma once

#include "ListeningSocket.hpp"

#include <string>

class ASimpleServer : public ListeningSocket
{
private:
    virtual void accepter(int listenFd) = 0;
    virtual int responder(int clientFd, const std::string &data) = 0;

public:
    ASimpleServer(int domain, int type, int protocol, int port, u_long ip, int backlog);
    virtual ~ASimpleServer(void);

    virtual void launch() = 0;

};
