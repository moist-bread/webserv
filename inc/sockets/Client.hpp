#pragma once

#include "CgiHandler.hpp"

// #include "../requests/Request.hpp"
#include "../requests/Response.hpp"

class Request;
// class Response;

class Client
{
private:
	int _ClientFd;
	time_t _lastActivity;
	
public:
	Client(void);					// default constructor
	Client(int fd);			   		// int constructor
	Client(Client const &source);	// copy constructor
	~Client(void);					// destructor
	Client &operator=(Client const &source); // copy assignment operator overload

	Request request;
	Response response;
	CgiHandler cgi;

	time_t GetLastActivity() const;
	int GetClientFd() const;

	void updateLastActivity();

};
