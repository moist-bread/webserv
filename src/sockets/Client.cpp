#include "../../inc/sockets/Client.hpp"
#include "../../inc/ansi_color_codes.h"

#include <ctime>	// time

Client::Client(void) : serverConfig(NULL), response(request)
{
	updateLastActivity();
}

Client::Client(int fd) : _ClientFd(fd), serverConfig(NULL), listenFd(-1), response(request)
{
	updateLastActivity();
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Client::Client(Client const &src) : response(request)
{
	*this = src;
	std::cout << GRN "the Client ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Client::~Client(void)
{
	std::cout << GRN "the Client ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Client &Client::operator=(Client const &src)
{
	if (this != &src)
	{
		this->_ClientFd = src._ClientFd;
		this->_lastActivity = src._lastActivity;
		
		this->request = src.request;
		this->response = src.response;
		this->cgi = src.cgi;
		this->listenFd = src.listenFd;
		this->serverConfig = src.serverConfig;
	}
	return (*this);
}

int Client::GetClientFd() const
{
	return _ClientFd;
}

time_t Client::GetLastActivity() const
{
	return _lastActivity;
}

void Client::updateLastActivity()
{
	this->_lastActivity = std::time(NULL);
}
