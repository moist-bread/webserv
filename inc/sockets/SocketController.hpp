#pragma once
#include "../Network.hpp"

class SocketController
{
protected:
    int _sock;
    int _connection;
    struct sockaddr_in _address;
public:
    SocketController(int domain,int type,int protocol,int port,u_long ip);
	SocketController(SocketController const &source);	// copy constructor
	~SocketController(void);				// destructor

    virtual int connect_to_network(int sock,struct sockaddr_in address) = 0;
    void test_connection(int test_connection);
	SocketController &operator=(SocketController const &source);

    //getter
    int getSocketfd(void) const;
    int getConnection(void) const;
    struct sockaddr_in getStructAdress(void)const;
};

std::ostream &operator<<(std::ostream &out, SocketController const &source);