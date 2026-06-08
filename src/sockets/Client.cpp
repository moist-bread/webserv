#include "../../inc/sockets/Client.hpp"
#include "../../inc/requests/Inspect.hpp"
#include "../../inc/ansi_color_codes.h"

#include <ctime>	// time

Client::Client(void) : serverConfig(NULL), request(serverConfig), response(request, serverConfig)
{
	// -------- THIS NEEDS TO BE REMOVED
	if (Inspect::debug)
	{
		std::cout << RED "the Client ";
		std::cout << "has been empty created" DEF << std::endl;
		sleep (2);
	}
	updateLastActivity();
}

Client::Client(int fd, int lfd, const ServerConfig *sc) : _ClientFd(fd), serverConfig(sc), listenFd(lfd), request(sc), response(request, sc)
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

Client::~Client(void)
{
	if (Inspect::debug)
	{
		std::cout << GRN "the Client ";
		std::cout << URED "has been deleted" DEF << std::endl;
	}
}

Client &Client::operator=(Client const &src)
{
	if (Inspect::debug)
	{
		std::cout << GRN "the Client ";
		std::cout << UYEL "has been copy ASSIGNED created" DEF << std::endl;
	}
	if (this != &src)
	{
		this->_ClientFd = src._ClientFd;
		this->_lastActivity = src._lastActivity;
		
		this->serverConfig = src.serverConfig;
		this->listenFd = src.listenFd;
		
		this->request = src.request;
		this->response = src.response;
		this->cgi = src.cgi;
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
