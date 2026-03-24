#include "../../inc/sockets/ListeningSocket.hpp"
#include "../../inc/ansi_color_codes.h"

ListeningSocket::ListeningSocket(int domain, int type, int protocol, int port, u_long ip, int backlog) : BindingSocket(domain, type, protocol, port, ip)
{
	std::cout << GRN "the ListeningSocket ";
	std::cout << UCYN "has been created" DEF << std::endl;
	this->_backlog = backlog;
	start_listening();
	test_connection(listening);
}

ListeningSocket::ListeningSocket(ListeningSocket const &source) : BindingSocket(source)
{
	*this = source;
	std::cout << GRN "the ListeningSocket ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

ListeningSocket::~ListeningSocket(void)
{
	std::cout << GRN "the ListeningSocket ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

void ListeningSocket::start_listening()
{
	this->listening = listen(this->_sock, this->_backlog);
}

ListeningSocket &ListeningSocket::operator=(ListeningSocket const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, ListeningSocket const &source)
{
	(void)source;
	out << BLU "ListeningSocket";
	out << DEF << std::endl;
	return (out);
}
