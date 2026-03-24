#pragma once
#include "SocketController.hpp"

class BindingSocket : public SocketController
{
private:
	int _binding;

public:
	BindingSocket(int domain, int type, int protocol, int port, u_long ip);
	BindingSocket(BindingSocket const &source); // copy constructor
	~BindingSocket(void);						// destructor

	BindingSocket &operator=(BindingSocket const &source); // copy assignment operator overload

	int connect_to_network(int sock, struct sockaddr_in address);
};

std::ostream &operator<<(std::ostream &out, BindingSocket const &source);