#include "../../inc/sockets/SocketController.hpp"
#include "../../inc/http/Inspect.hpp"
#include "../../inc/ansi_color_codes.h"

#include <iostream>

/*
	@param domain   Address family (e.g. AF_INET for IPv4).
 * @param type     Socket type (e.g. SOCK_STREAM for TCP).
 * @param protocol Protocol (typically 0 to auto-select).
 * @param port     Port number in host byte order; converted internally with htons.
 * @param ip       IP address in host byte order (e.g. INADDR_ANY); converted with htonl.
*/
SocketController::SocketController(int domain, int type, int protocol, int port, u_long ip)
	: _sock(socket(domain, type, protocol))
{
	test_connection(_sock);

	// adding an option so that the socket can be reused
	// SOL_SOCKET: sets the option on API level
	// SO_REUSEADDR: allows adress to be reused without cooldown
	// optval: 1 enable, 0 disable
	int optval = 1;
	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	// Populate the address structure used by bind
	this->_address.sin_family = domain;

	// htons converts the port from host to network byte order
	this->_address.sin_port = htons(port);

	// htonl converts the IPv4 address from host to network byte order.
	this->_address.sin_addr.s_addr = htonl(ip);
}

SocketController::SocketController(SocketController const &source) : _sock(source.getSocketfd()) { *this = source; }

SocketController &SocketController::operator=(SocketController const &source)
{
	if (this != &source)
	{
		_sock = source.getSocketfd();
		_address = source.getStructAdress();
	}
	return (*this);
}

SocketController::~SocketController(void) {}

void SocketController::test_connection(int test_connection)
{
	if (0 > test_connection)
	{
		std::cout << "Trying to connect..." << std::endl;
		throw (std::runtime_error("Connection failed"));
	}
}

int SocketController::getSocketfd(void) const {	return this->_sock; }

struct sockaddr_in SocketController::getStructAdress(void) const { return _address; }
