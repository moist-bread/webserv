#include "../../inc/sockets/ListeningSocket.hpp"

/**
 * @brief Constructs a fully operational listening socket.
 *
 * Delegates address setup and binding to BindingSocket, then calls
 * start_listening() to invoke listen with the given backlog, and
 * validates the result via test_connection().
 *
 * @param domain   Address family (e.g. AF_INET).
 * @param type     Socket type (e.g. SOCK_STREAM).
 * @param protocol Protocol (typically 0).
 * @param port     Port number in host byte order.
 * @param ip       IP address in host byte order (e.g. INADDR_ANY).
 * @param backlog  Maximum length of the pending connection queue passed to listen(.
 *
 * @throws std::runtime_error if listen returns a failure value.
 */
ListeningSocket::ListeningSocket(int domain, int type, int protocol, int port, u_long ip, int backlog) : BindingSocket(domain, type, protocol, port, ip)
{
	this->_backlog = backlog;
	start_listening();
	test_connection(listening);
}

ListeningSocket::ListeningSocket(ListeningSocket const &source) : BindingSocket(source) { *this = source; }

ListeningSocket &ListeningSocket::operator=(ListeningSocket const &source)
{
	if (this != &source)
		(void)source;
	return (*this);
}

ListeningSocket::~ListeningSocket(void) {}

void ListeningSocket::start_listening()
{
	this->listening = listen(this->_sock, this->_backlog);
}
