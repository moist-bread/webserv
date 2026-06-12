#include "../../inc/sockets/Client.hpp"

#include <ctime> // time

Client::Client(int fd, int lfd, const ServerConfig *sc) : _ClientFd(fd), request(sc), response(request, sc), cgi(), serverConfig(sc), listenFd(lfd)
{
	updateLastActivity();
}

Client::Client(Client const &src) : request(src.serverConfig), response(request, src.serverConfig) { *this = src; }

Client &Client::operator=(Client const &src)
{
	if (this != &src)
	{
		this->_lastActivity = src.getLastActivity();
		this->_ClientFd = src.getClientFd();

		this->request = src.request;
		this->response = src.response;
		this->cgi = src.cgi;
		
		this->serverConfig = src.serverConfig;
		this->listenFd = src.listenFd;
	}
	return (*this);
}

Client::~Client(void) {}

const time_t &Client::getLastActivity() const { return (_lastActivity); }

int Client::getClientFd() const { return (_ClientFd); }

void Client::updateLastActivity() { this->_lastActivity = std::time(NULL); }
