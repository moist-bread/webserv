#include "../../inc/sockets/Client.hpp"
#include "../../inc/http/Inspect.hpp"

#include "../../inc/ansi_color_codes.h"

#include <iostream>
#include <ctime> // time

Client::Client(int fd, int lfd, const ServerConfig *sc) : _ClientFd(fd), request(sc), response(request, sc), cgi(), serverConfig(sc), listenFd(lfd)
{
	updateLastActivity();
	if (Inspect::debug)
	{
		std::cout << GRN "the Client ";
		std::cout << UCYN "has been created" DEF << std::endl;
	}
}

Client::Client(Client const &src) : request(src.serverConfig), response(request, src.serverConfig)
{
	*this = src;
	if (Inspect::debug)
	{
		std::cout << GRN "the Client ";
		std::cout << UYEL "has been copy created" DEF << std::endl;
	}
}

Client::~Client(void) {}

Client &Client::operator=(Client const &src)
{
	if (Inspect::debug)
	{
		std::cout << GRN "the Client ";
		std::cout << UYEL "has been copy ASSIGNED created" DEF << std::endl;
	}
	if (this != &src)
	{
		this->_ClientFd = src.getClientFd();
		this->_lastActivity = src.getLastActivity();

		this->serverConfig = src.serverConfig;
		this->listenFd = src.listenFd;

		this->request = src.request;
		this->response = src.response;
		this->cgi = src.cgi;
	}
	return (*this);
}

const time_t &Client::getLastActivity() const { return (_lastActivity); }

int Client::getClientFd() const { return (_ClientFd); }

void Client::updateLastActivity()
{
	this->_lastActivity = std::time(NULL);
}
