#pragma once

#include "ListeningSocket.hpp"

class ASimpleServer : public ListeningSocket
{
private:
    ListeningSocket *_socket;
    virtual void accepter(int listenFd) = 0;
    virtual int responder(int clientFd, const std::string &data) = 0;

public:
    // -- MISSING DEFAULT CONSTRUCTOR 
    // ASimpleServer(void);
    ASimpleServer(int domain, int type, int protocol, int port, u_long ip, int backlog);
    virtual ~ASimpleServer(void);               // destructor
    ASimpleServer(ASimpleServer const &source); // copy constructor
    ASimpleServer &operator=(ASimpleServer const &source); // copy assignment operator overload

    virtual void launch() = 0;
    ListeningSocket *getSocket() const;

};
