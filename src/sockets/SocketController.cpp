#include "../../inc/sockets/SocketController.hpp"

#include "../../inc/ansi_color_codes.h"

#include <stdio.h>	// perror
#include <stdlib.h>	// exit

/*

domain
Escolhe qual maneira de comunicacao se vai usar.
Escolhi AF_INET porque e que usa os IPv4 que vai ser preciso para a socket futuramente acho eu LMAO

type
Pede que tipo de esquema e que queres que a tua socket comunique,
eu escolhi a SOCK_STREAM porque e a mais completa e segura

protocol
Faz com que a tua socket possa ter comportamentos diferentes do normal (ativa flags que estao no manual)
por enquanto vamos manter desativado para testar

*/

SocketController::SocketController(void) {}

SocketController::SocketController(int domain, int type, int protocol, int port, u_long ip)
	: _sock(socket(domain, type, protocol))
{
	std::cout << GRN "the SocketController ";
	std::cout << UCYN "has been created" DEF << std::endl;

	test_connection(_sock);

	// adding an option so that the socket can be reused
	// SOL_SOCKET: sets the option on API level
	// SO_REUSEADDR: allows adress to be reused without cooldown
	// optval: 1 enable, 0 disable
	int optval = 1;
	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	// Init na estrutura de network para configurar a conexao feita atraves do bind
	this->_address.sin_family = domain;

	// temos de usar htons para organizar os bites de port para se encaxarem bem na network
	this->_address.sin_port = htons(port);

	// IPV4 da maquina
	this->_address.sin_addr.s_addr = htonl(ip);
}

void SocketController::test_connection(int test_connection)
{
	if (0 > test_connection)
	{
		perror("Trying to connect...");
		// -- CAN'T USE EXIT!!
		exit(EXIT_FAILURE);
	}
}

SocketController::SocketController(SocketController const &source)
{
	*this = source;
}

SocketController &SocketController::operator=(SocketController const &source)
{
	if (this != &source)
	{
		// -- NOT FULLY WORKING
		// _sock = source.getSocketfd();
		_address = source.getStructAdress();
	}
	return (*this);
}

std::ostream &operator<<(std::ostream &out, SocketController const &source)
{
	out << BLU "SocketController " << "_sockedfd value: " << source.getSocketfd();
	out << DEF << std::endl;
	return (out);
}

int SocketController::getSocketfd(void) const 
{
	return this->_sock;
}

struct sockaddr_in SocketController::getStructAdress(void) const
{
	return _address;
}

SocketController::~SocketController(void) {}
