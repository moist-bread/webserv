#include "../../inc/sockets/Client.hpp"
#include "../../inc/ansi_color_codes.h"

#include <ctime>	// time

Client::Client(void)
{
	updateLastActivity();
}

Client::Client(int fd) : _ClientFd(fd)
{
	updateLastActivity();
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Client::Client(Client const &source)
{
	*this = source;
	std::cout << GRN "the Client ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Client::~Client(void)
{
	std::cout << GRN "the Client ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Client &Client::operator=(Client const &source)
{
	if (this != &source)
	{
		this->_ClientFd = source._ClientFd;
		this->_lastActivity = source._lastActivity;
		this->request = source.request;
		this->request = source.request;
		this->response = source.response;
		this->cgi = source.cgi;
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
