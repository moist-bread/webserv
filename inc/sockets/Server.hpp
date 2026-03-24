#pragma once
#include "../Network.hpp"

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
	std::map<int, Client> _clients;
	std::vector<struct pollfd> _pollfds; // vou dar store no fd das sockets que vao ser criadas quando alguem se conectar

	std::vector<int> _serverPorts; // Fds de todas as portas abertas
	std::vector<ListeningSocket *> _extraListeners;
	std::vector<int> _listeningFds;

	std::map<int, int> _cgiMap; // <Fd_do_Tubo_CGI, Fd_do_Cliente>

	void SetupPorts();
	void SetNonblocking(int fd);

	void PopulatePollInfo(int fd);
	void removeClient(int fd, size_t &index);

	void launch();
	bool isServerSocket(int fd);
	ConnectionStatus getStatus(int ret);

	void accepter(int listenFd);
	int responder(int clientFd, const std::string &data);

	void recieveCgiOutput(int fd, size_t *pollfds_idx);
	void recieveClientRequest(int fd, size_t *pollfds_idx);
	void sendClientResponse(int fd, size_t *pollfds_idx);
	void inactivityTimeout(int fd, size_t *pollfds_idx);

public:
	Server(void);				  // default constructor
	Server(Server const &source); // copy constructor
	~Server(void);				  // destructor

	Server &operator=(Server const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, Server const &source);
