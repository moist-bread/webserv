#include "../../inc/sockets/ASimpleServer.hpp"

ASimpleServer::ASimpleServer(int domain, int type, int protocol, int port, u_long ip, int backlog) : ListeningSocket(domain, type, protocol, port, ip, backlog)
{
	this->_backlog = backlog;
}

ASimpleServer::~ASimpleServer(void) {}

ASimpleServer::ASimpleServer(ASimpleServer const &source) : ListeningSocket(source)
{
	*this = source;
}

ASimpleServer &ASimpleServer::operator=(ASimpleServer const &source)
{
	if (this != &source)
		(void)source;
	return (*this);
}

ListeningSocket *ASimpleServer::getSocket() const
{
	return this->_socket;
}
