#pragma once
#include "ListeningSocket.hpp"

class ASimpleServer : public ListeningSocket
{
private:
    ListeningSocket *_socket;
    virtual void accepter(int listenFd) = 0;
    virtual int responder(int clientFd, const std::string &data) = 0;

public:
    ASimpleServer(int domain, int type, int protocol, int port, u_long ip, int backlog);
    ASimpleServer(ASimpleServer const &source); // copy constructor
    virtual ~ASimpleServer(void);               // destructor

    virtual void launch() = 0;
    ListeningSocket *getSocket() const;

    ASimpleServer &operator=(ASimpleServer const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, ASimpleServer const &source);
