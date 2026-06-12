#pragma once

#include "SocketController.hpp"

class BindingSocket : public SocketController
{
private:
	int _binding;

public:
	BindingSocket(int domain, int type, int protocol, int port, u_long ip);
	BindingSocket(BindingSocket const &source);
	BindingSocket &operator=(BindingSocket const &source);
	~BindingSocket(void);

	int connect_to_network(int sock, struct sockaddr_in address);
	int get_binding(void) const;
};
