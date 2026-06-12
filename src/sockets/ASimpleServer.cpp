#include "../../inc/sockets/ASimpleServer.hpp"

ASimpleServer::ASimpleServer(int domain, int type, int protocol, int port, u_long ip, int backlog) : ListeningSocket(domain, type, protocol, port, ip, backlog)
{
	this->_backlog = backlog;
}

ASimpleServer::~ASimpleServer(void) {}
