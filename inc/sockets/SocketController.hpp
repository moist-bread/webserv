#pragma once

#include "../../inc/sockets/FileDescriptor.hpp"

#include <netinet/in.h>
#include <iostream>

class SocketController
{
protected:
    FileDescriptor _sock;
    struct sockaddr_in _address;

public:
    SocketController(void);
    SocketController(int domain, int type, int protocol, int port, u_long ip);
    SocketController(SocketController const &source); // copy constructor
    virtual ~SocketController(void);                  // destructor

    virtual int connect_to_network(int sock, struct sockaddr_in address) = 0;
    void test_connection(int test_connection);
    SocketController &operator=(SocketController const &source);

    // getter
    int getSocketfd(void) const;
    struct sockaddr_in getStructAdress(void) const;
};

std::ostream &operator<<(std::ostream &out, SocketController const &source);
