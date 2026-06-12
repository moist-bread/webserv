#include "../../inc/sockets/BindingSocket.hpp"

BindingSocket::BindingSocket(int domain, int type, int protocol, int port, u_long ip) : SocketController(domain, type, protocol, port, ip)
{
	this->_binding = connect_to_network(getSocketfd(), getStructAdress());
	test_connection(_binding);
}

BindingSocket::BindingSocket(BindingSocket const &source) : SocketController(source) { *this = source; }

BindingSocket &BindingSocket::operator=(BindingSocket const &source)
{
	if (this != &source)
		this->_binding = get_binding();
	return (*this);
}

BindingSocket::~BindingSocket(void) {}

int BindingSocket::connect_to_network(int sock, struct sockaddr_in address)
{
	return bind(sock, (struct sockaddr *)&address, sizeof(address));
}

int BindingSocket::get_binding(void) const { return (_binding); }
