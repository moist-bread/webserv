#include "../../inc/sockets/BindingSocket.hpp"

#include "../../inc/ansi_color_codes.h"

BindingSocket::BindingSocket(int domain, int type, int protocol, int port, u_long ip) : SocketController(domain, type, protocol, port, ip)
{
	std::cout << GRN "the BindingSocket ";
	std::cout << UCYN "has been created" DEF << std::endl;

	this->_binding = connect_to_network(getSocketfd(), getStructAdress());
	test_connection(_binding);
}

BindingSocket::BindingSocket(BindingSocket const &source) : SocketController(source)
{
	*this = source;
}

BindingSocket &BindingSocket::operator=(BindingSocket const &source)
{
	if (this != &source)
		(void)source;
	return (*this);
}

int BindingSocket::connect_to_network(int sock, struct sockaddr_in address)
{
	return bind(sock, (struct sockaddr *)&address, sizeof(address));
}

BindingSocket::~BindingSocket(void) {}
