#include "../../inc/sockets/Client.hpp"


Client::Client(void) 
{
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Client::Client(int fd) : _ClientFd(fd)
{
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

void Client::feed(const char* data, int size)
{
    _requestBuffer.append(data, size);
}

std::string Client::GetRequestBuffer() const
{
	return _requestBuffer;
}

std::string Client::GetWriteBuffer() const
{
	return _respondBuffer;
}

void Client::ClearRequestBuffer()
{
	this->_requestBuffer.clear();
}

void Client::ClearRespondBuffer()
{
	this->_respondBuffer.clear();
}

void Client::SetRespondBuffer(const std::string& str)
{
	this->_respondBuffer = str;
}

bool Client::requestFullyReceived()
{
	//Parse dos headers normais
	if(_requestBuffer.find("\r\n\r\n") != std::string::npos)
	{
		//Eventualmente vou ter de voltar aqui para ver de outros headers de outros eventos
		std::cout << "End of request from client ..." << std::endl;
		return true;
	}
	return false;
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
		this->_ClientFd = source._ClientFd;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, Client const &source)
{
	(void)source;
	out << BLU "Client";
	out << DEF << std::endl;
	return (out);
}

