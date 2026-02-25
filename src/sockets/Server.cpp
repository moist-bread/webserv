#include "../../inc/sockets/Server.hpp"

static int getPrimaryPort()
{
	static const int ports[] = {8080,9090,9094};
	return ports[0];
}

Server::Server(void) : ASimpleServer(AF_INET,SOCK_STREAM,0,getPrimaryPort(),INADDR_ANY,10)
{
	std::cout << GRN "the Server ";
	std::cout << UCYN "has been created" DEF << std::endl;

	SetupPorts();
	launch();
}

void Server::SetupPorts()
{
	//Portas hardcoded para testar
	static const int ports[] = {8080,9090,9094};
	const int arraySize = sizeof(ports) / sizeof(ports[0]);

	_serverPorts.clear();
	_extraListeners.clear();
	_listeningFds.clear();

	std::cout << "Showing off my ports" << std::endl;
	for (int i = 0; i < arraySize; i++)
	{
		_serverPorts.push_back(ports[i]);
		std::cout << "Index: " <<  i << " Port Number: " << ports[i] << std::endl;
	}

	for (int i = 0; i < arraySize; i++)
	{
		//Sou a primeira socket aka first Listener
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

void Server::accepter(int listenFd)
{
	struct sockaddr_in clientAddr;
	int addrlen = sizeof(clientAddr);

	//Aceita nova conexao
	int newFd = accept(listenFd,(struct sockaddr*)&clientAddr,(socklen_t *)&addrlen);
	if (newFd < 0)
	{
		perror("accept");
		return;
	}

	//
	PopulatePollInfo(newFd);

	Client newClient(newFd);

	_clients.insert(std::make_pair(newFd,newClient));
	std::cout << "Cliente criado na socket " << newFd<< std::endl;
}

void Server::handler(std::string buffer)
{
    std::cout << buffer << std::endl;
}

int Server::responder(int clientFd,const std::string& data)
{
    return write(clientFd, data.c_str(), data.size());
}

void Server::SetNonblocking(int fd)
{

	/*
	
		FLAG PROIBIDA F_GETFL REMOVER FUTURAMENTE
	
	*/
	//Vamos verificar se tem alguma flag de erro no fd da socket
	int flags = fcntl(fd,F_GETFL,0);
	if(flags == -1)
	{
		perror("fnctl F_GETFL");
		return;
	}
	//Fixe esta tudo bem vamos passar para NONBLOCKING
	// | O_NONBLOCK adiciona nova flag sem remover outras
	if(fcntl(fd,F_SETFL,flags | O_NONBLOCK) == -1)
	{
		perror("Something went wrong while passing to NONBLOCKING");
	}
}

void Server::PopulatePollInfo(int fd)
{
	//Vamos passar a socket para nonblock para ficar sempre a escuta
	SetNonblocking(fd);

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	_pollfds.push_back(pfd);
}

ConnectionStatus getStatus(int ret)
{
    if(0 > ret)
        return IO_ERROR;
    if(0 == ret)
        return IO_CLOSED;
    return IO_DATA_READY;
}

std::string Server::OpenFile(const std::string& path)
{

	//Abre ficheiro
	std::ifstream file(path.c_str());
	if(!file.is_open())
	{
		perror("File does not exist");
	}

	std::string ContentOfFile;

	std::cout << "Correr ate agora" << std::endl;
	std::stringstream buffer;
	
	//Copia o conteudo todo do ficheiro para dentro de um buffer
	buffer << file.rdbuf();

	//Vamos returnar o conteudo do buffer aka ficheiro yipeeee
	return buffer.str();
}

bool Server::isServerSocket(int fd)
{
	for (size_t j = 0; j < _listeningFds.size(); j++)
	{
		if (_listeningFds[j] == fd)
		{
			return true;
		}
	}
	return false;
}

void Server::launch()
{
    while (true)
	{
		int pollCount = poll(&_pollfds[0],_pollfds.size(),-1);
		if(0 > pollCount)
		{
			std::cout << "Timeout..." << std::endl;
		}

		for (size_t i = 0; i < _pollfds.size(); i++)
		{
    		int fd = _pollfds[i].fd;

			// --- CASO 1: NOVA CONEXÃO ---
			if(isServerSocket(fd) && (_pollfds[i].revents == POLLIN))
			{
				accepter(fd);
				continue; // Vai para o próximo fd
			}

			// --- CASO 2: LEITURA (CLIENTE MANDA REQUEST) ---
			else if(_pollfds[i].revents == POLLIN)
			{
				char tmp[65536]; //64kb por segundo
				int ret = recv(fd, tmp, sizeof(tmp), 0);
				
				ConnectionStatus status = getStatus(ret);
				if (status == IO_ERROR || status == IO_CLOSED) {
					removeClient(fd, i);
					continue;
				}

				//Passa a informaçao para o buffer do cliente
				_clients[fd].feed(tmp, ret);
				//std::cout << _clients[fd].GetRequestBuffer() << std::endl;
				std::cout << "Bytes recebidos: " << _clients[fd].GetRequestBuffer().size() << "\r" << std::flush;
				
				if(_clients[fd].requestFullyReceived())
				{

				//Vou ler o ficheiro 
				std::string content;
				content = OpenFile("src/sockets/index.html");

				//Erro 404
				if(content.empty())
				{
					std::cout << "File is empty maybe use another file" << std::endl;
				}
				
				std::cout << content << std::endl;

				//Vou criar uma resposta para enviar :)

				std::string body = content;

				std::stringstream ss;
				ss << "HTTP/1.1 200 OK\r\n";
				ss << "Content-Length: " << body.size() << "\r\n";
				ss << "\r\n";
				ss << body;

				std::string response = ss.str();

				_clients[fd].SetRespondBuffer(response);

				// Paramos de escutar POLLIN e passamos a escutar POLLOUT
				_pollfds[i].events = POLLOUT;
			}
    	}

		// --- CASO 3: ESCRITA (SERVIDOR ENVIANDO RESPOSTA) ---
		else if(_pollfds[i].revents == POLLOUT)
		{

			int bytesWritten;
			
			bytesWritten = responder(fd,_clients[fd].GetWriteBuffer());
			
			//Vou apagar o que ja li do buffer pois ja nao e preciso
			//Para isso vou pegar a posicao inicial e ate a parte que li
			//Tenho de limpar os dados que o buffer ja leu
			
			if(bytesWritten > 0)
			{
				_clients[fd].EraseParte(0,bytesWritten);

				//Buffer vazio, vamos voltar a escutar para input
				if(_clients[fd].GetWriteBuffer().empty())
				{
					_pollfds[i].events = POLLIN;
					//Cleaning
					_clients[fd].ClearRequestBuffer(); 
				}
			}
			else if(0 > bytesWritten)
			{
				removeClient(fd,i);
			}
		}
	}
	}		
		std::cout << "== DONE ===" << std::endl;
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
	{
		delete _extraListeners[i];
	}
}

Server &Server::operator=(Server const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, Server const &source)
{
	(void)source;
	out << BLU "Server";
	out << DEF << std::endl;
	return (out);
}

void Server::removeClient(int fd, size_t& index)
{
    _clients.erase(fd);
    close(fd);
    _pollfds.erase(_pollfds.begin() + index);
    index--;
}
