#include "../../inc/sockets/Client.hpp"

Client::Client(void)
{
	updateLastActivity();
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Client::Client(int fd) : _ClientFd(fd)
{
	updateLastActivity();
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

// Getter

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
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
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

std::ostream &operator<<(std::ostream &out, Client const &source)
{
	(void)source;
	out << BLU "Client";
	out << DEF << std::endl;
	return (out);
}
