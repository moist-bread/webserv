#pragma once

#include "FileDescriptor.hpp"

#include <netinet/in.h> // sockaddr_in

class SocketController
{
protected:
    FileDescriptor _sock;
    struct sockaddr_in _address;

public:
    SocketController(int domain, int type, int protocol, int port, u_long ip);
    SocketController(SocketController const &source);
    SocketController &operator=(SocketController const &source);
    virtual ~SocketController(void);

    virtual int connect_to_network(int sock, struct sockaddr_in address) = 0;
    void test_connection(int test_connection);

    int getSocketfd(void) const;
    struct sockaddr_in getStructAdress(void) const;
};
