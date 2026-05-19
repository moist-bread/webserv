#include "../../inc/sockets/Server.hpp"
#include "../../inc/sockets/Client.hpp"

#include "../../inc/ansi_color_codes.h"

#include <unistd.h> // close, read
#include <ctime>	// time, difftime
#include <stdio.h>	// perror
#include <fcntl.h>	// fcntl

#define TIMEOUT_TIME 120
#define READ_BUFFER_SIZE 65536 // 64kb per second

extern bool running;

/*
static int getPrimaryPort()
{
	// will be replaced by config file
	static const int ports[] = {8080, 9090, 9094};
	return ports[0];
}
*/

Server::Server(Config config) : ASimpleServer(AF_INET, SOCK_STREAM, 0, config.getServers()[0].port, INADDR_ANY, 10), _config(config)
{
	std::cout << GRN "the Server ";
	std::cout << UCYN "has been created" DEF << std::endl;
	
	SetupPorts();
	launch();
}

Server::Server(Server const &source) : ASimpleServer(source), _config(source._config)
{
    *this = source;
    _fdToServerConfig.clear();
    const std::vector<ServerConfig>& servers = _config.getServers();
    for (size_t i = 0; i < _listeningFds.size() && i < servers.size(); i++)
        _fdToServerConfig[_listeningFds[i]] = &servers[i];
}

Server::~Server(void)
{
	std::cout << GRN "the Server ";
	std::cout << URED "has been deleted" DEF << std::endl;
	for (size_t i = 0; i < _extraListeners.size(); i++)
		delete _extraListeners[i];
}

Server &Server::operator=(Server const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}


void Server::SetupPorts()
{
    const std::vector<ServerConfig>& servers = _config.getServers();
    std::map<int, int> portToFd; // porta | fd já criado

    for (size_t i = 0; i < servers.size(); i++)
    {
        int port = servers[i].port;
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

	//Debug
	std::cout << "[DEBUG] SetupPorts — mapa fd→config:" << std::endl;
    for (std::map<int, const ServerConfig*>::iterator it = _fdToServerConfig.begin(); it != _fdToServerConfig.end(); it++)
        std::cout << "  fd=" << it->first << " porta=" << it->second->port << " serverName=" << (it->second->serverNames.empty() ? "none" : it->second->serverNames[0]) << std::endl;
}

void Server::SetNonblocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		perror("Something went wrong while passing to NONBLOCKING");
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

void Server::removeClient(int fd, size_t &index)
{
	std::cout << RED "REMOVING THE CLIENT " DEF << fd << "\n";
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
const ServerConfig* Server::resolveServerConfig(int listenFd, const std::string &hostHeader) const
{
    const std::vector<ServerConfig>& servers = _config.getServers();

    // 1ª passagem — procura pelo serverName exato
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (_listeningFds[i] != listenFd)
            continue;
        const std::vector<std::string>& names = servers[i].serverNames;
        for (size_t j = 0; j < names.size(); j++)
            if (names[j] == hostHeader)
                return &servers[i];
    }

    // 2ª passagem — fallback: primeiro servidor que escuta neste fd
    for (size_t i = 0; i < _listeningFds.size(); i++)
        if (_listeningFds[i] == listenFd)
            return &servers[i];

    return NULL;
}

void Server::launch()
{
	if(_pollfds.empty())
	{
 		std::cerr << "[Server] Nenhuma porta configurada — a sair." << std::endl;
		return;
	}

	running = true;
	while (running)
	{
		int pollCount = poll(&_pollfds[0], _pollfds.size(), -1);
		if (pollCount <= 0)
		{
			// research what to do when it timesout
			std::cout << "Timeout... " << pollCount << std::endl;
		}

		for (size_t i = 0; i < _pollfds.size(); i++)
		{
			int fd = _pollfds[i].fd;

			if (!running && _clients.find(fd) != _clients.end())
			{
				removeClient(fd, i);
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
		std::cout << "== DONE ===" << std::endl;
	}
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
        return (perror("accept"));

    PopulatePollInfo(newFd);
    Client newClient(newFd);


	newClient.listenFd = listenFd;
    // Associa o ServerConfig correto ao cliente
    if (_fdToServerConfig.count(listenFd))
        newClient.serverConfig = _fdToServerConfig[listenFd];

    _clients.insert(std::make_pair(newFd, newClient));
	std::cout << "Cliente criado na socket " << newFd << std::endl;

	if (newClient.serverConfig)
    	std::cout << "[DEBUG] accepter — cliente " << newFd << " associado ao servidor porta=" << newClient.serverConfig->port << std::endl;
    else
        std::cout << "[DEBUG] accepter — cliente " << newFd << " SEM serverConfig!" << std::endl;
}

int Server::responder(int clientFd, const std::string &data)
{
	return write(clientFd, data.c_str(), data.size());
}

void Server::recieveCgiOutput(int fd, size_t *pollfds_idx)
{
	int clientFd = _cgiMap[fd];

	_clients[clientFd].cgi.time_started = VALUE_NOT_SET;

	// Aconteceu algo de errado com o pipe se entrou aqui ou o processo fez KABUM
	if (_pollfds[*pollfds_idx].revents & POLLERR)
	{
		std::cerr << "[CGI] POLLERR no pipe — a gerar resposta de erro\n";
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--;
		_clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;

		for (size_t j = 0; j < _pollfds.size(); j++)
			if (_pollfds[j].fd == clientFd)
			{
				_pollfds[j].events = POLLOUT;
				break;
			}
		return;
	}

	//  Ler do tubo
	char buffer[READ_BUFFER_SIZE];
	int bytesRead = read(fd, buffer, READ_BUFFER_SIZE);

	if (bytesRead > 0)
	{
		_clients[clientFd].response.cgi_reply = std::string(buffer, bytesRead);
		for (size_t j = 0; j < _pollfds.size(); j++)
		{
			if (_pollfds[j].fd == clientFd)
			{
				_pollfds[j].events = POLLOUT;
				break;
			}
		}
		// !! maybe send right away here with encoding chunked
		// and then later send the finishing chunck when it gets the 0
	}
	else if (bytesRead == 0) // 0 = EOF, -1 aqui só pode ser erro real (poll garantiu que havia dados)
	{
		std::cout << "CGI terminou de processar para o cliente " << clientFd << std::endl;
		_clients[clientFd].response.cgi_reply.clear();

		//  Fechar e limpar o tubo
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
		if (_clients[clientFd].response.cgi_reply.empty() && !_clients[clientFd].response.is_chunked)
		{
			std::cerr << "[CGI] Sem output — a gerar resposta de erro\n";
			_clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;
		}

		// Acordar o Cliente! Passar o cliente para POLLOUT para ele enviar a resposta
		for (size_t j = 0; j < _pollfds.size(); j++)
		{
			if (_pollfds[j].fd == clientFd)
			{
				_pollfds[j].events = POLLOUT;
				break;
			}
		}
		// std::cout << YEL "-- CGI Response --" DEF "\n\n";
		// std::cout << YEL "Body..." DEF << std::endl;
		// std::cout << _clients[clientFd].response.cgi_reply << std::endl;
	}
	else // will see if all of this is necessary in the future
	{
		std::cout << "CGI read error " << clientFd << std::endl;
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--;
		_clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;

		for (size_t j = 0; j < _pollfds.size(); j++)
			if (_pollfds[j].fd == clientFd)
			{
				_pollfds[j].events = POLLOUT;
				break;
			}
		return;
	}
}

bool allowedCGI(std::string cgi_type, std::string loc)
{
	(void)loc;
	if (cgi_type == "py")
		return (true);
	return (false);
}

void Server::recieveClientRequest(int fd, size_t *pollfds_idx)
{
	char tmp[READ_BUFFER_SIZE];
	int ret = recv(fd, tmp, READ_BUFFER_SIZE, 0);

	ConnectionStatus status = getStatus(ret);
	if (status == IO_ERROR || status == IO_CLOSED)
		return (removeClient(fd, *pollfds_idx));

	std::string rec = std::string(tmp, ret);
	std::cout << std::endl;
	// std::cout << "Raw Request:" << std::endl;
	// std::cout << rec << std::endl;
	std::cout << "Bytes recebidos: " << ret << std::endl;

	_clients[fd].response.status_code = OK;
	try
	{
		_clients[fd].request.process(rec);
		std::string hostHeader = _clients[fd].request.headers["host"];

		size_t colon = hostHeader.find(':');
		if (colon != std::string::npos)
			hostHeader = hostHeader.substr(0, colon);

		_clients[fd].serverConfig = resolveServerConfig(_clients[fd].listenFd, hostHeader);
	if (_clients[fd].serverConfig)
    	std::cout << "[DEBUG] resolveServerConfig — host='" << hostHeader << "' resolveu para porta=" << _clients[fd].serverConfig->port << std::endl;
	else
    	std::cout << "[DEBUG] resolveServerConfig — host='" << hostHeader << "' NAO resolveu!" << std::endl;
	}
	catch (const Request::ParseError &e)
	{
		std::cerr << e.what() << std::endl;
		_clients[fd].request.set_state(END);
		_clients[fd].response.status_code = e.request_status;
		_pollfds[*pollfds_idx].events = POLLOUT; // switch to sending
		return;
	}

	_clients[fd].updateLastActivity();

	if (_clients[fd].request.get_state() != END)
	{
		std::cout << BLU "Missing request parts, waiting for more...\n" DEF;
		return;
	}

	// start the cgi execution right away
	if (allowedCGI(_clients[fd].request.file_extension, _clients[fd].request.path_uri))
	{
		try
		{
			_clients[fd].cgi.process(_clients[fd].request);
			int contentOfCgiFd = _clients[fd].cgi.getPipeOutReadFd();
			PopulatePollInfo(contentOfCgiFd);
			_cgiMap.insert(std::make_pair(contentOfCgiFd, _clients[fd].GetClientFd()));
			_pollfds[*pollfds_idx].events = 0; // O cliente fica "adormecido" no poll até o CGI acabar
			return;
		}
		catch (const CgiHandler::CgiExecutionFail &e)
		{
			std::cerr << "cgi failure: " << e.what() << '\n';
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
		_clients[fd].updateLastActivity();
		std::cout << YEL "response sent..." DEF << std::endl;
		_clients[fd].response.full_response.erase(0, bytesWritten);
		if (_clients[fd].response.full_response.empty())
		{
			// only continue onto the next request after being able to
			// send everything from the previously processed request
			std::cout << YEL "full response is empty, change to POLLIN" DEF << std::endl;
			_clients[fd].response.set_state(PREP);
			_pollfds[*pollfds_idx].events = POLLIN;
		}
	}
	else if (bytesWritten < 0)
	{
		removeClient(fd, *pollfds_idx);
	}
}

void Server::inactivityTimeout(int fd, size_t *pollfds_idx)
{
	if (_clients[fd].response.headers.find("Connection") != _clients[fd].response.headers.end())
		if (_clients[fd].response.headers["Connection"] == "close")
			return (removeClient(fd, *pollfds_idx));

	// Chegámos ao fim do processamento deste FD nesta volta.
	// Vamos verificar se ele está "morto" há demasiado tempo.
	time_t now = std::time(NULL);
	double seconds_idle = std::difftime(now, _clients[fd].GetLastActivity());

	// Vamos definir TIMEOUT_TIME segundos como o limite máximo de inatividade
	if (seconds_idle > TIMEOUT_TIME)
	{
		std::cout << "\n[TIMEOUT] O cliente " << fd << " inativo há " << seconds_idle << " segundos. A desconectar..." << std::endl;
		return (removeClient(fd, *pollfds_idx));
	}

	if (_clients[fd].cgi.getCgiActivityStart() == VALUE_NOT_SET)
		return;
	double cgi_time = std::difftime(now, _clients[fd].cgi.getCgiActivityStart());
	if (cgi_time > TIMEOUT_TIME)
	{
		std::cout << "\n[TIMEOUT] Cgi Timed out " << fd << " inativo há " << cgi_time << " segundos. A desconectar..." << std::endl;
		return (removeClient(fd, *pollfds_idx));
	}
}

std::ostream &operator<<(std::ostream &out, Server const &source)
{
	(void)source;
	out << BLU "Server";
	out << DEF << std::endl;
	return (out);
}
