#pragma once

#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "CgiHandler.hpp"

struct ServerConfig;

class Client
{
private:
	time_t _lastActivity;
	int _ClientFd;

public:
	Client(int fd, int lfd, const ServerConfig *sc);
	Client(Client const &src);
	~Client(void);
	Client &operator=(Client const &src);
	
	Request request;
	Response response;
	CgiHandler cgi;

	const ServerConfig *serverConfig;
	int listenFd;
	
	const time_t &getLastActivity() const;
	int getClientFd() const;

	void updateLastActivity();
};
