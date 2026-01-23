#pragma once
#include "SocketController.hpp"

class ConnectingSocket : public SocketController
{
	private:
		int _connectSocket;
public:
	ConnectingSocket(int domain,int type,int protocol,int port,u_long ip);
	ConnectingSocket(ConnectingSocket const &source);	// copy constructor
	~ConnectingSocket(void);				// destructor

	ConnectingSocket &operator=(ConnectingSocket const &source); // copy assignment operator overload

	int connect_to_network(int sock,struct sockaddr_in address);
};

std::ostream &operator<<(std::ostream &out, ConnectingSocket const &source);