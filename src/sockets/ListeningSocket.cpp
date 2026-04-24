#include "../../inc/sockets/ListeningSocket.hpp"

ListeningSocket::ListeningSocket(int domain, int type, int protocol, int port, u_long ip, int backlog) : BindingSocket(domain, type, protocol, port, ip)
{
	this->_backlog = backlog;
	start_listening();
	test_connection(listening);
}

ListeningSocket::ListeningSocket(ListeningSocket const &source) : BindingSocket(source)
{
	*this = source;
}

ListeningSocket::~ListeningSocket(void) {}

void ListeningSocket::start_listening()
{
	this->listening = listen(this->_sock, this->_backlog);
}

ListeningSocket &ListeningSocket::operator=(ListeningSocket const &source)
{
	if (this != &source)
		(void)source;
	return (*this);
}
