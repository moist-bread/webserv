#include "../../inc/sockets/Client.hpp"

Client::Client(void) 
{
	this->_readAllHeaders = false;
	this->_headerBytes = 0;
	this->_contentLength = 0;
	updateLastActivity();
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Client::Client(int fd) : _ClientFd(fd)
{
	this->_readAllHeaders = false;
	this->_headerBytes = 0;
	this->_contentLength = 0;
	updateLastActivity();
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

void Client::feed(const char* data, int size)
{
    _requestBuffer.append(data, size);
	updateLastActivity();
}

//Getter

std::string Client::GetRequestBuffer() const
{
	return _requestBuffer;
}
int Client::GetClientFd() const
{
	return _ClientFd;
}

std::string Client::GetWriteBuffer() const
{
	return _respondBuffer;
}

time_t Client::GetLastActivity() const
{
	return _lastActivity;
}

//Setter
void Client::SetRespondBuffer(const std::string& str)
{
	this->_respondBuffer = str;
}

void Client::AppendRespondBuffer(const std::string& str)
{
	this->_respondBuffer.append(str);
}

void Client::updateLastActivity()
{
	this->_lastActivity = std::time(NULL);
}


// Cleaning
void Client::ClearRequestBuffer()
{
	this->_requestBuffer.clear();
	this->_readAllHeaders = false;
    this->_headerBytes = 0;
    this->_contentLength = 0;
}

void Client::ClearRespondBuffer()
{
	this->_respondBuffer.clear();
}

void Client::EraseParte(int start,int idx)
{
	if(start >= 0 && start + idx <= (int)_respondBuffer.size())
	{
		this->_respondBuffer.erase(start,idx);
	}
	
}

void Client::extractContentLength()
{
    // Procura por "Content-Length: "
    size_t pos = _requestBuffer.find("Content-Length: ");
    
    if (pos != std::string::npos)
    {
        // Avança o ponteiro para o início do número
        pos += 16; 
        
        // Converte o texto para número (usando atoi do C, que é C++98 safe)
        _contentLength = std::atoi(_requestBuffer.c_str() + pos);
        std::cout << "O servidor espera um Body de " << _contentLength << " bytes." << std::endl;
    }
    else
    {
        _contentLength = 0; // Se não tem Content-Length, assumimos que não tem body
    }
}

bool Client::requestFullyReceived()
{
	//Parse dos headers normais
	if(!_readAllHeaders)
	{
		size_t pos = _requestBuffer.find("\r\n\r\n");
		if(pos != std::string::npos)
		{
			//Acabei os headers
			_headerBytes = pos + 4;

			_readAllHeaders = true;

			extractContentLength();
		}
	}
	
	if(_readAllHeaders)
	{
		//Nao tenho body sou um humilde GET
		if(_contentLength == 0)
		{
			return true;
		}

		size_t currentbodySize = _requestBuffer.size() - _headerBytes;
		if(currentbodySize >= _contentLength)
		{
			std::cout << "Post recebido com sucesso, tamanho do body: " << currentbodySize << std::endl;
			return true;
		} 
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
	{
		this->_ClientFd = source._ClientFd;
		this->_requestBuffer = source._requestBuffer;
		this->_respondBuffer = source._respondBuffer;
		this->_readAllHeaders = source._readAllHeaders;
		this->_headerBytes = source._headerBytes;
		this->_contentLength = source._contentLength;
		this->_lastActivity = source._lastActivity;
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
