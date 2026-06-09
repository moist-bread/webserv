#pragma once

#include "ASimpleServer.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include "../../inc/serverConfig/Config.hpp"
#include "../../inc/http/Inspect.hpp"
#include <map>
#include <vector>
#include <iostream>
#include <poll.h>

class Client;

enum ConnectionStatus
{
	IO_ERROR,
	IO_CLOSED,
	IO_DATA_READY,
	IO_DATA_OUT
};

class Server : public ASimpleServer
{
private:
	Config _config;

	std::map<int, Client> _clients;
	std::vector<struct pollfd> _pollfds; // vou dar store no fd das sockets que vao ser criadas quando alguem se conectar

	std::vector<int> _serverPorts; // Fds de todas as portas abertas
	std::vector<ListeningSocket *> _extraListeners;
	std::vector<int> _listeningFds;

	std::map<int, int> _cgiMap; // <Fd_do_Tubo_CGI, Fd_do_Cliente>

	std::map<int, const ServerConfig *> _fdToServerConfig; // listenFd → config do servidor

	void SetNonblocking(int fd);

	void PopulatePollInfo(int fd);
	void removeClient(int fd, size_t &index, const t_remove_reason &reason);

	void launch();
	bool isServerSocket(int fd);
	ConnectionStatus getStatus(int ret);

	// const ServerConfig* resolveServerConfig(int listenFd, const std::string &hostHeader) const;

	void closeCgiConnection(const int &fd, size_t *pollfds_idx);
	void switchToPollout(const int &fd);
	void SetupPorts();
	void accepter(int listenFd);
	int responder(int clientFd, const std::string &data);

	void recieveCgiOutput(int fd, size_t *pollfds_idx);
	void recieveClientRequest(int fd, size_t *pollfds_idx);
	void sendClientResponse(int fd, size_t *pollfds_idx);
	void inactivityTimeout(int fd, size_t *pollfds_idx);

	Client &get_corresponding_client(const int &fd);

public:
	Server(Config config);		  // default constructor
	Server(Server const &source); // copy constructor
	~Server(void);				  // destructor

	Server &operator=(Server const &source); // copy assignment operator overload
	const std::map<int, const ServerConfig *> &get_fdToServerConfig() const;
};

std::ostream &operator<<(std::ostream &out, const Server &src);
