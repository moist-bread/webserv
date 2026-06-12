#pragma once

#include "ASimpleServer.hpp"
#include "../../inc/serverConfig/Config.hpp"
#include "../../inc/http/Inspect.hpp"
#include <poll.h> // pollfd

class Client;
struct ServerConfig;

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
	Config _config; //Global configuration object parsed from the .conf file

	std::map<int, Client> _clients; //Maps active client socket FDs to their respective Client objects.
	std::vector<struct pollfd> _pollfds; // Vector of FDs monitored by the central poll() loop.

	std::vector<int> _serverPorts; // List of all unique ports the server is currently listening on
	std::vector<ListeningSocket *> _extraListeners; //Pointers to secondary listening sockets created during setup.
	std::vector<int> _listeningFds; //Vector of all listening socket FDs (used to detect new connections).

	std::map<int, int> _cgiMap; //  Maps a CGI's output pipe FD to the FD of the Client who requested it. <Fd_do_Tubo_CGI, Fd_do_Cliente>

	std::map<int, const ServerConfig *> _fdToServerConfig; // Maps a listening socket FD to its logical Virtual Server configuration. listenFd → config do servidor

	void SetupPorts();
	
	void PopulatePollInfo(int fd);
	void SetNonblocking(int fd);
	
	void removeClient(int fd, size_t &index, const t_remove_reason &reason);
	void closeCgiConnection(const int &fd, size_t *pollfds_idx);
	void switchToPollout(const int &fd);
	
	void launch();
	
	void accepter(int listenFd);
	void recieveCgiOutput(int fd, size_t *pollfds_idx);
	void recieveClientRequest(int fd, size_t *pollfds_idx);
	void sendClientResponse(int fd, size_t *pollfds_idx);
	void inactivityTimeout(int fd, size_t *pollfds_idx);
	
	
public:
	Server(Config config);
	Server(Server const &source);
	Server &operator=(Server const &source);
	~Server(void);
	
	bool isServerSocket(int fd);
	ConnectionStatus getStatus(int ret);
	int responder(int clientFd, const std::string &data);
	Client &get_corresponding_client(const int &fd);

	const std::map<int, const ServerConfig *> &get_fdToServerConfig() const;
};

std::ostream &operator<<(std::ostream &out, const Server &src);
