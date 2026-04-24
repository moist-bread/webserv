#include "../../inc/sockets/Server.hpp"

extern bool running;

static int getPrimaryPort()
{
	// will be replaced by config file
	static const int ports[] = {8080, 9090, 9094};
	return ports[0];
}

Server::Server(void) : ASimpleServer(AF_INET, SOCK_STREAM, 0, getPrimaryPort(), INADDR_ANY, 10)
{
	std::cout << GRN "the Server ";
	std::cout << UCYN "has been created" DEF << std::endl;

	SetupPorts();
	launch();
}

Server::Server(Server const &source) : ASimpleServer(source)
{
	*this = source;
	std::cout << GRN "the Server ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
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
	// Portas hardcoded para testar
	static const int ports[] = {8080, 9090, 9094};
	const int arraySize = sizeof(ports) / sizeof(ports[0]);

	_serverPorts.clear();
	_extraListeners.clear();
	_listeningFds.clear();

	std::cout << "Showing off my ports" << std::endl;
	for (int i = 0; i < arraySize; i++)
	{
		_serverPorts.push_back(ports[i]);
		std::cout << "Index: " << i << " Port Number: " << ports[i] << std::endl;
	}

	for (int i = 0; i < arraySize; i++)
	{
		// Sou a primeira socket aka first Listener
		if (i == 0)
		{
			_listeningFds.push_back(this->_sock);
			PopulatePollInfo(this->_sock);
			continue;
		}

		ListeningSocket *listener = new ListeningSocket(AF_INET, SOCK_STREAM, 0, ports[i], INADDR_ANY, 10);
		_extraListeners.push_back(listener);
		int fd = listener->getSocketfd();
		_listeningFds.push_back(fd);
		PopulatePollInfo(fd);
	}
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
	for (std::map<int, int>::iterator it = _cgiMap.begin(); it !=_cgiMap.end(); it++)
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

void Server::launch()
{
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

	// Aceita nova conexao
	int newFd = accept(listenFd, (struct sockaddr *)&clientAddr, (socklen_t *)&addrlen);
	if (newFd < 0)
		return (perror("accept"));

	PopulatePollInfo(newFd);
	Client newClient(newFd);
	_clients.insert(std::make_pair(newFd, newClient));
	std::cout << "Cliente criado na socket " << newFd << std::endl;
}

int Server::responder(int clientFd, const std::string &data)
{
	return write(clientFd, data.c_str(), data.size());
}


void Server::recieveCgiOutput(int fd, size_t *pollfds_idx)
{
	int clientFd = _cgiMap[fd];
 
	_clients[clientFd].cgi.time_started = -1;
 
	//Aconteceu algo de errado com o pipe se entrou aqui ou o processo fez KABUM
	if (_pollfds[*pollfds_idx].revents & POLLERR)
	{
		std::cerr << "[CGI] POLLERR no pipe — a gerar resposta de erro\n";
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--;
		_clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;
		_clients[clientFd].response.process(_clients[clientFd].request);
		for (size_t j = 0; j < _pollfds.size(); j++)
			if (_pollfds[j].fd == clientFd)
				{
					_pollfds[j].events = POLLOUT;
					 break;
				}
		return;
	}
 
	//  Ler do tubo
	// CHANGE IT SO THAT BUFFER SIZE IS DIFF DEPENDING ON CONFIG?
	char buffer[4096];
	int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
 
	if (bytesRead > 0)
	{
		_clients[clientFd].response.full_response += std::string(buffer, bytesRead);
	}
	else if (bytesRead <= 0) // 0 = EOF, -1 aqui só pode ser erro real (poll garantiu que havia dados)
	{
		std::cout << "CGI terminou de processar para o cliente " << clientFd << std::endl;
 
		//  Fechar e limpar o tubo
		close(fd);
		_cgiMap.erase(fd);
		_pollfds.erase(_pollfds.begin() + *pollfds_idx);
		(*pollfds_idx)--; // Ajustar o índice porque apagámos um elemento do vector

		/*Fix
			Erro de antes como o python estava a crashar o pipeOUT estava a receber EOF com 0 bytes
			O cliente acordava todo feliz a pensar que tinha uma response so que ela tinha 0 bytes!

			Agora se o full_response estiver vazio apos o amigo CGI terminar significa que o script
			falhou no puro silencio shhhh e ainda recebe um 500 banger
		*/
		if (_clients[clientFd].response.full_response.empty())
        {
            std::cerr << "[CGI] Sem output — a gerar resposta de erro\n";
            _clients[clientFd].response.status_code = INTERNAL_SERVER_ERROR;
            _clients[clientFd].response.process(_clients[clientFd].request);
        }

		// Acordar o Cliente! Passar o cliente para POLLOUT para ele receber a resposta
		for (size_t j = 0; j < _pollfds.size(); j++)
		{
			if (_pollfds[j].fd == clientFd)
			{
				_pollfds[j].events = POLLOUT;
				break;
			}
		}
	}
}



void Server::recieveClientRequest(int fd, size_t *pollfds_idx)
{
	// CHANGE IT SO THAT BUFFER SIZE IS DIFF DEPENDING ON CONFIG
	// make it slightly bigger than the max to see if it overflows
	char tmp[65536]; // 64kb por segundo
	int ret = recv(fd, tmp, sizeof(tmp), 0);

	ConnectionStatus status = getStatus(ret);
	if (status == IO_ERROR || status == IO_CLOSED)
		return (removeClient(fd, *pollfds_idx));

	std::string rec = std::string(tmp, ret);
	std::cout << std::endl;
	std::cout << "Raw Request:" << std::endl;
	std::cout << rec << std::endl;
	std::cout << "Bytes recebidos: " << ret << std::endl;
	// HERE CHECK IF IT THE BUFFER WAS TOO SMALL

	_clients[fd].response.status_code = OK;
	try
	{
		_clients[fd].request.process(rec);
	}
	catch (const Request::ParseError &e)
	{
		std::cerr << e.what() << std::endl;
		_clients[fd].response.status_code = e.request_status;
	}

	if (_clients[fd].request.missing_request_part) 
	{
		// will probably change this when i implement chuncked requests
		std::cout << RED "WE ARE MISSING SOMETHING\n" DEF;
		return ;
	}

	std::cout << BLU "file extension: " DEF << _clients[fd].request.file_extension << std::endl;
	if (_clients[fd].request.file_extension == "py")
	{
		std::cout << BLU "THIS IS A CGI!!!!!!!\n" DEF;
		try
		{
			_clients[fd].cgi.process(_clients[fd].request);
			int contentOfCgiFd = _clients[fd].cgi.getPipeOutReadFd();
			PopulatePollInfo(contentOfCgiFd);
			_cgiMap.insert(std::make_pair(contentOfCgiFd, _clients[fd].GetClientFd()));
			// O cliente fica "adormecido" no poll até o CGI acabar
			_pollfds[*pollfds_idx].events = 0;
			return;
		}
		catch (const CgiHandler::CgiExecutionFail &e)
		{
			std::cerr << "cgi failure: " << e.what() << '\n';
			_clients[fd].response.status_code = INTERNAL_SERVER_ERROR;
		}
	}
	_clients[fd].response.process(_clients[fd].request);
	// Paramos de escutar POLLIN e passamos a escutar POLLOUT
	_clients[fd].updateLastActivity();
	_pollfds[*pollfds_idx].events = POLLOUT;
}

void Server::sendClientResponse(int fd, size_t *pollfds_idx)
{
	int bytesWritten = responder(fd, _clients[fd].response.full_response);
	if (bytesWritten > 0)
	{
		_clients[fd].updateLastActivity();
		_clients[fd].response.eraseWritten(0, bytesWritten);

		// ??? shouldnt the case of the buffer not being fully sent
		// handled by chunk responses????

		// Buffer vazio, vamos voltar a escutar para input
		if (_clients[fd].response.full_response.empty())
			_pollfds[*pollfds_idx].events = POLLIN;
	}
	else if (bytesWritten < 0)
	{
		removeClient(fd, *pollfds_idx);
	}
}

void Server::inactivityTimeout(int fd, size_t *pollfds_idx)
{
	if (_clients[fd].response.headers.find("connection") != _clients[fd].response.headers.end())
		if (_clients[fd].response.headers["connection"] == "close")
			return (removeClient(fd, *pollfds_idx));

	// Chegámos ao fim do processamento deste FD nesta volta.
	// Vamos verificar se ele está "morto" há demasiado tempo.
	time_t now = std::time(NULL);
	double seconds_idle = std::difftime(now, _clients[fd].GetLastActivity());

	// Vamos definir 120 segundos como o limite máximo de inatividade
	if (seconds_idle > TIMEOUT_TIME)
	{
		std::cout << "\n[TIMEOUT] O cliente " << fd << " inativo há " << seconds_idle << " segundos. A desconectar..." << std::endl;
		return (removeClient(fd, *pollfds_idx));
	}

	if (_clients[fd].cgi.getCgiActivityStart() == -1)
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
