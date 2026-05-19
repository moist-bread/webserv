#pragma once

#include "../requests/Request.hpp"
#include "../requests/Response.hpp"
#include "../serverConfig/ServerConfig.hpp"
#include "CgiHandler.hpp"

class Client
{
private:
	int _ClientFd;
	time_t _lastActivity;
	
public:
	const ServerConfig* serverConfig;
	int listenFd; // qual listener aceitou esta conexao

	Client(void);					// default constructor
	Client(int fd);			   		// int constructor
	Client(Client const &src);	// copy constructor
	~Client(void);					// destructor
	Client &operator=(Client const &src); // copy assignment operator overload

	Request request;
	Response response;
	CgiHandler cgi;

	time_t GetLastActivity() const;
	int GetClientFd() const;

	void updateLastActivity();
};
