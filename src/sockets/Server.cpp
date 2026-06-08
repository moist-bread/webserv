#include "../../inc/sockets/Server.hpp"
#include "../../inc/sockets/Client.hpp"

#include "../../inc/ansi_color_codes.h"

#include <unistd.h> // close, read
#include <ctime>	// time, difftime
#include <string.h>	// strerror
#include <errno.h>	// errno
#include <fcntl.h>	// fcntl

#define TIMEOUT_TIME 120
#define READ_BUFFER_SIZE 65536 // 64kb per second

extern bool running;

Server::Server(Config config) : ASimpleServer(AF_INET, SOCK_STREAM, 0, config.getallServers()[0].getListenPort(), INADDR_ANY, 10), _config(config)
{
	if (Inspect::debug)
	{
		std::cout << GRN "the Server ";
		std::cout << UCYN "has been created" DEF << std::endl;
	}
	
	SetupPorts();
	launch();
}

Server::Server(Server const &source) : ASimpleServer(source), _config(source._config)
{
    *this = source;
    _fdToServerConfig.clear();
    const std::vector<ServerConfig>& servers = _config.getallServers();
    for (size_t i = 0; i < _listeningFds.size() && i < servers.size(); i++)
        _fdToServerConfig[_listeningFds[i]] = &servers[i];
}

Server::~Server(void)
{
	if (Inspect::debug)
	{
		std::cout << GRN "the Server ";
		std::cout << URED "has been deleted" DEF << std::endl;
	}

	for (size_t i = 0; i < _extraListeners.size(); i++)
		delete _extraListeners[i];
}

Server &Server::operator=(Server const &source)
{
	if (Inspect::debug)
		std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}


void Server::SetupPorts()
{
    const std::vector<ServerConfig>& servers = _config.getallServers();
    std::map<int, int> portToFd; // porta | fd já criado

    for (size_t i = 0; i < servers.size(); i++)
    {
        int port = servers[i].getListenPort();
        int fd;

        if (i == 0)
        {
            fd = this->_sock;
            _listeningFds.push_back(fd);
            PopulatePollInfo(fd);
            portToFd[port] = fd;
        }
        else if (portToFd.count(port)) // porta ja existe
        {
            fd = portToFd[port]; // reutiliza o fd existente
            _listeningFds.push_back(fd);
        }
        else
        {
            ListeningSocket *listener = new ListeningSocket(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY, 10);
            _extraListeners.push_back(listener);
            fd = listener->getSocketfd();
            _listeningFds.push_back(fd);
            PopulatePollInfo(fd);
            portToFd[port] = fd;
        }

        _serverPorts.push_back(port);
        _fdToServerConfig[fd] = &servers[i];
    }


	Inspect::inspect_server_activity("started up", *this);
}

void Server::SetNonblocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		// --------------- what to do here
		std::cout << "Something went wrong while passing to NONBLOCKING" << std::endl;
	}
}

void Server::PopulatePollInfo(int fd)
{
	// Vamos passar a socket para nonblock para ficar sempre a escuta
	SetNonblocking(fd);

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	_pollfds.push_back(pfd);
}

void Server::removeClient(int fd, size_t &index, const t_remove_reason &reason)
{
	Inspect::inspect_removed_cl(reason, fd);
	_clients.erase(fd);
	for (std::map<int, int>::iterator it = _cgiMap.begin(); it != _cgiMap.end(); it++)
	{
		if (it->second == fd)
		{
			_cgiMap.erase(it);
			break;
		}
	}
	close(fd);
	_pollfds.erase(_pollfds.begin() + index);
	index--;
}


/*
Descobre qual ServerConfig responde ao pedido com base no ListenFd en Host: Header
*/
// const ServerConfig* Server::resolveServerConfig(int listenFd, const std::string &hostHeader) const
// {
//     const std::vector<ServerConfig>& servers = _config.getallServers();

//     // 1ª passagem — procura pelo serverName exato
//     for (size_t i = 0; i < servers.size(); i++)
//     {
//         if (_listeningFds[i] != listenFd)
//             continue;
//         const std::vector<std::string>& names = servers[i].serverNames;
//         for (size_t j = 0; j < names.size(); j++)
//             if (names[j] == hostHeader)
//                 return &servers[i];
//     }

//     // 2ª passagem — fallback: primeiro servidor que escuta neste fd
//     for (size_t i = 0; i < _listeningFds.size(); i++)
//         if (_listeningFds[i] == listenFd)
//             return &servers[i];

//     return NULL;
// }

void Server::launch()
{
	if(_pollfds.empty())
		return (Inspect::inspect_server_activity("no ports configured, shuting down", *this));

	running = true;
	while (running)
	{
		int pollCount = poll(&_pollfds[0], _pollfds.size(), -1);
		if (pollCount <= 0)
		{
			// research what to do when it timesout

			// !! zero indicates that the system call timed out before any file descriptors became ready
			// !! On error, -1
			std::cout << std::endl;
			if (pollCount == 0) 
				Inspect::inspect_server_activity("timed out", *this);
			else 
				Inspect::inspect_server_activity( "been stopped in poll by: " + std::string(strerror(errno)), *this);
		}

		for (size_t i = 0; i < _pollfds.size(); i++)
		{
			int fd = _pollfds[i].fd;

			if (!running && _clients.find(fd) != _clients.end())
			{
				removeClient(fd, i, SERVER_CLOSE);
				continue;
			}

			// --- CASO 1: NOVA CONEXÃO ---
			if (isServerSocket(fd) && (_pollfds[i].revents & POLLIN))
				accepter(fd);

			// --- CASO X: CGI ---
			else if (_cgiMap.find(fd) != _cgiMap.end() && (_pollfds[i].revents & (POLLIN | POLLHUP | POLLERR)))
				recieveCgiOutput(fd, &i);

			// --- CASO 2: LEITURA (CLIENTE MANDA REQUEST) ---
			else if (_pollfds[i].revents & (POLLIN | POLLHUP | POLLERR))
				recieveClientRequest(fd, &i);

			// --- CASO 3: ESCRITA (SERVIDOR ENVIANDO RESPOSTA) ---
			if (_pollfds[i].revents & POLLOUT)
				sendClientResponse(fd, &i);

			// --- CASO 4: DEFESA CONTRA TIMEOUTS ---
			if (!isServerSocket(fd) && _clients.find(fd) != _clients.end())
				inactivityTimeout(fd, &i);
		}
		// std::cout << "== DONE ===" << std::endl;
	}
	Inspect::inspect_server_activity("shut down", *this);
}

bool Server::isServerSocket(int fd)
{
	for (size_t j = 0; j < _listeningFds.size(); j++)
	{
		if (_listeningFds[j] == fd)
			return true;
	}
	return false;
}

ConnectionStatus Server::getStatus(int ret)
{
	if (ret < 0)
		return IO_ERROR;
	if (ret == 0)
		return IO_CLOSED;
	return IO_DATA_READY;
}

void Server::accepter(int listenFd)
{
    struct sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);

    int newFd = accept(listenFd, (struct sockaddr *)&clientAddr, (socklen_t *)&addrlen);
    if (newFd < 0)
		return (Inspect::inspect_client_activity("failed to be accepted", newFd, 0));

	if (!_fdToServerConfig.count(listenFd))
		throw (std::runtime_error("cliente sem server config"));
		
	PopulatePollInfo(newFd);
	Client newClient(newFd, listenFd, _fdToServerConfig[listenFd]);
    _clients.insert(std::make_pair(newFd, newClient));
	Inspect::inspect_client_activity("been accepted", newFd, newClient.serverConfig->getListenPort());

	if (Inspect::debug)
	{
		std::cout << RED "this is the config\n" DEF;
		std::cout << *_fdToServerConfig[listenFd] << std::endl;
	}
}

int Server::responder(int clientFd, const std::string &data)
{
	return write(clientFd, data.c_str(), data.size());
}

void Server::recieveCgiOutput(int fd, size_t *pollfds_idx)
{
	int clientFd = _cgiMap[fd];


	// !! FOUND PROBLEM: ele aqui cria um cliente novo VAZIO quando o cliente
	// !! que estava associado ao CGI deixa de existir através do operador [] do mapa

	// -- possivel solução
	if (_clients.find(clientFd) == _clients.end())
	{
		if (Inspect::debug)
		{
			std::cout << RED "Cliente associated with this cgi has been disconnected..." << std::endl;
		}
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--;
		for (size_t j = 0; j < _pollfds.size(); j++)
		{
			if (_pollfds[j].fd == clientFd)
			{
				_pollfds[j].events = POLLOUT;
				break;
			}
		}
		return;
	}
	_clients[clientFd].cgi.setCgiActivityStart(std::time(NULL));

	// Aconteceu algo de errado com o pipe se entrou aqui ou o processo fez KABUM
	if (_pollfds[*pollfds_idx].revents & POLLERR)
	{
		Inspect::inspect_cgi_activity("failed to execute due to a POLLERR error", clientFd);

		// !! close cgi connection
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--;
		_clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;

		// !! switch to sending response (POLLOUT)
		for (size_t j = 0; j < _pollfds.size(); j++)
		{
			if (_pollfds[j].fd == clientFd)
			{
				_pollfds[j].events = POLLOUT;
				break;
			}
		}
		return;
	}

	char buffer[READ_BUFFER_SIZE];
	int bytesRead = read(fd, buffer, READ_BUFFER_SIZE);

	if (bytesRead > 0)
	{
		// -- recieved content -> send it and wait for more
		_clients[clientFd].response.cgi_reply = std::string(buffer, bytesRead);
	}
	else if (bytesRead == 0)
	{
		// -- recieved EOF -> send to signal that the cgi ended
		_clients[clientFd].cgi.setCgiActivityStart(VALUE_NOT_SET);
		_clients[clientFd].response.cgi_reply.clear();

		// !! close cgi connection
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--; // Ajustar o índice porque apagámos um elemento do vector

		/*
			Erro de antes: como o python estava a crashar o pipeOUT estava a receber EOF com 0 bytes
			O cliente acordava todo feliz a pensar que tinha uma response so que ela tinha 0 bytes!

			Fix: agora se o full_response estiver vazio apos o CGI terminar significa que o script
			falhou silenciosamente e ainda recebe um INTERNAL_SERVER_ERROR
		*/
		if (!_clients[clientFd].response.is_chunked)
		{
			Inspect::inspect_cgi_activity("failed to execute due to an error", clientFd);
			_clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;
		}
		Inspect::inspect_cgi_activity("finished execution", clientFd);
	}
	else
	{
		// -- read failed -> send error response
		_clients[clientFd].cgi.setCgiActivityStart(VALUE_NOT_SET);
		Inspect::inspect_cgi_activity("failed to execute due to a read error", clientFd);

		// !! close cgi connection
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--;
		_clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;
	}

	// !! switch to sending response (POLLOUT)
	for (size_t j = 0; j < _pollfds.size(); j++)
	{
		if (_pollfds[j].fd == clientFd)
		{
			_pollfds[j].events = POLLOUT;
			break;
		}
	}
}

void Server::recieveClientRequest(int fd, size_t *pollfds_idx)
{
	char tmp[READ_BUFFER_SIZE];
	int ret = recv(fd, tmp, READ_BUFFER_SIZE, 0);

	ConnectionStatus status = getStatus(ret);
	if (status == IO_ERROR || status == IO_CLOSED)
		return (removeClient(fd, *pollfds_idx, RECV_FAIL));

	std::string rec = std::string(tmp, ret);
	
	/* std::cout << std::endl;
	std::cout << "Raw Request:" << std::endl;
	std::cout << rec << std::endl; */
	if (Inspect::debug)
		std::cout << "Bytes recebidos: " << ret << std::endl;

	_clients[fd].response.status_code = OK;
	try
	{
		_clients[fd].request.process(rec);
		
		// std::string hostHeader = _clients[fd].request.headers["host"];
		// size_t colon = hostHeader.find(':');
		// if (colon != std::string::npos)
		// 	hostHeader = hostHeader.substr(0, colon);

		// _clients[fd].serverConfig = resolveServerConfig(_clients[fd].listenFd, hostHeader);
		// if (_clients[fd].serverConfig)
		// 	std::cout << "[DEBUG] resolveServerConfig — host='" << hostHeader << "' resolveu para porta=" << _clients[fd].serverConfig->port << std::endl;
		// else
		// 	std::cout << "[DEBUG] resolveServerConfig — host='" << hostHeader << "' NAO resolveu!" << std::endl;
	}
	catch (const Request::ParseError &e)
	{
		Inspect::inspect_request_activity(e.what(), _clients[fd].request);
		_clients[fd].request.set_state(END);
		_clients[fd].response.status_code = e.request_status;
		_pollfds[*pollfds_idx].events = POLLOUT;
		return;
	}

	_clients[fd].updateLastActivity();

	if (_clients[fd].request.get_state() != END)
	{
		// std::cout << BLU "Missing request parts, waiting for more...\n" DEF;
		return;
	}


	Inspect::inspect_request_activity("", _clients[fd].request);
	// start the cgi execution right away
	if (!_clients[fd].request.loc->getCgiExecutable(_clients[fd].request.file_extension).empty())
	{
		try
		{
			Inspect::inspect_cgi_activity("started execution", fd);
			_clients[fd].cgi.process(_clients[fd].request);
			int contentOfCgiFd = _clients[fd].cgi.getPipeOutReadFd();
			PopulatePollInfo(contentOfCgiFd);
			_cgiMap.insert(std::make_pair(contentOfCgiFd, _clients[fd].GetClientFd()));
			_pollfds[*pollfds_idx].events = 0; // O cliente fica "adormecido" no poll até o CGI acabar
			return;
		}
		catch (const CgiHandler::CgiExecutionFail &e)
		{
			Inspect::inspect_cgi_activity(e.what(), fd);
			_clients[fd].response.status_code = INTERNAL_SERVER_ERROR;
		}
	}

	_pollfds[*pollfds_idx].events = POLLOUT; // switch to sending
}

void Server::sendClientResponse(int fd, size_t *pollfds_idx)
{
	_clients[fd].response.process();
	int bytesWritten = responder(fd, _clients[fd].response.full_response);
	if (bytesWritten > 0)
	{
		// std::cout << BLU "response sent..." DEF << std::endl;
		_clients[fd].updateLastActivity();
		_clients[fd].response.full_response.erase(0, bytesWritten);
		if (_clients[fd].response.full_response.empty())
		{
			// only continue onto the next request after being able to
			// send everything from the previously processed request
			Inspect::inspect_response_activity("", _clients[fd].response);
			_clients[fd].response.set_state(PREP);
			_pollfds[*pollfds_idx].events = POLLIN;
		}
	}
	else if (bytesWritten < 0)
	{
		removeClient(fd, *pollfds_idx, WRITE_FAIL);
	}
}

void Server::inactivityTimeout(int fd, size_t *pollfds_idx)
{
	if (_clients[fd].response.headers.find("Connection") != _clients[fd].response.headers.end())
		if (_clients[fd].response.headers["Connection"] == "close")
			return (removeClient(fd, *pollfds_idx, CLOSE_CONNECTION));

	// Chegámos ao fim do processamento deste FD nesta volta.
	// Vamos verificar se ele está "morto" há demasiado tempo.
	time_t now = std::time(NULL);
	time_t seconds_idle = std::difftime(now, _clients[fd].GetLastActivity());

	// Vamos definir TIMEOUT_TIME segundos como o limite máximo de inatividade
	if (seconds_idle > TIMEOUT_TIME)
		return (removeClient(fd, *pollfds_idx, TIMEOUT));

	if (_clients[fd].cgi.getCgiActivityStart() == VALUE_NOT_SET)
		return;
	seconds_idle = std::difftime(now, _clients[fd].cgi.getCgiActivityStart());
	if (seconds_idle > TIMEOUT_TIME)
		return (removeClient(fd, *pollfds_idx, TIMEOUT_CGI));
}

const std::map<int, const ServerConfig*> &Server::get_fdToServerConfig() const { return (_fdToServerConfig); };

std::ostream &operator<<(std::ostream &out, const Server &src)
{
	out << MAG "-- Server Information --" DEF << std::endl;

	out << MAG "Fds → Server Configurations..." DEF << std::endl;
	for (std::map<int, const ServerConfig*>::const_iterator it = src.get_fdToServerConfig().begin(); it != src.get_fdToServerConfig().end(); it++)
	{
		out << MAG "    [" << (*it).first << "]" DEF;
		out << " | Port=" << (*it).second->getListenPort(); // ---------- server name PENDING
		out << " | Server Name=" << ((*it).second->serverNames.empty() ? "none" : (*it).second->serverNames[0]) << " |" << std::endl;

	}
	return (out);
}
