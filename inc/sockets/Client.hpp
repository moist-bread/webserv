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
	const ServerConfig *serverConfig;
	int listenFd; // qual listener aceitou esta conexao
	
	Request request;
	Response response;
	CgiHandler cgi;

	Client(void);
	Client(int fd, int lfd, const ServerConfig *sc);
	Client(Client const &src);
	~Client(void);
	Client &operator=(Client const &src);

	time_t GetLastActivity() const;
	int GetClientFd() const;

	void updateLastActivity();
};
