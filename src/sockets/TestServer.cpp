#include "../../inc/sockets/TestServer.hpp"

TestServer::TestServer(void) : ASimpleServer(AF_INET,SOCK_STREAM,0,PORT,INADDR_ANY,10)
{
	std::cout << GRN "the TestServer ";
	std::cout << UCYN "has been created" DEF << std::endl;
	launch();
}

//Vamos ignorar o control + c yupiii
void signalHandler()
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	
}

void TestServer::accepter()
{
    int addrlen = sizeof(this->_address);

	//Aceita nova conexao
    int newFd = accept(this->_sock,(struct sockaddr*)&this->_address,(socklen_t *)&addrlen);

	//
	PopulatePollInfo(newFd);

	Client newClient(newFd);

	_clients.insert(std::make_pair(newFd,newClient));

	std::cout << "Cliente criado na socket " << newFd<< std::endl;
}

void TestServer::handler(std::string buffer)
{
    std::cout << buffer << std::endl;
}

void TestServer::responder(int clientFd,const std::string& data)
{
    write(clientFd, data.c_str(), data.size());
}

void TestServer::SetNonblocking(int fd)
{

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
		perror("Something went wrong wgile passing to NONBLOCKING");
	}
}

void TestServer::PopulatePollInfo(int fd)
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

std::string TestServer::OpenFile(const std::string& path)
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

void TestServer::launch()
{

	PopulatePollInfo(this->_sock);

	//Hmmmm meio que sem o control + c nao tenho maneira de parar o loop 
	//signalHandler();
    while (true)
	{

		//std::cout << "=== WAITING ===" << std::endl;
		int pollCount = poll(&_pollfds[0],_pollfds.size(),-1);
		if(0 > pollCount)
		{
			std::cout << "Timeout..." << std::endl;
		}

		for (size_t i = 0; i < _pollfds.size(); i++)
		{
    	int fd = _pollfds[i].fd;

		// --- CASO 1: NOVA CONEXÃO ---
		if(fd == this->_sock && (_pollfds[i].revents & POLLIN))
		{
			accepter();
			continue; // Vai para o próximo fd
		}

    	// --- CASO 2: LEITURA (CLIENTE MANDA REQUEST) ---
		else if(_pollfds[i].revents & POLLIN)
		{
			char tmp[1024];
			int ret = recv(fd, tmp, sizeof(tmp), 0);
			
			ConnectionStatus status = getStatus(ret);
			if (status == IO_ERROR || status == IO_CLOSED) {
				removeClient(fd, i);
				continue;
			}

			//Passa a informaçao para o buffer do cliente
			_clients[fd].feed(tmp, ret);
			std::cout << _clients[fd].GetRequestBuffer() << std::endl;

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
		else if(_pollfds[i].revents & POLLOUT)
		{
			
			responder(fd,_clients[fd].GetWriteBuffer());
			
			// Volta para modo input
			_pollfds[i].events = POLLIN;


			_clients[fd].ClearRequestBuffer(); // Limpa o buffer para o próximo request
			_clients[fd].ClearRespondBuffer();
		}
	}
	}		
		std::cout << "== DONE ===" << std::endl;
	}
	


 
TestServer::TestServer(TestServer const &source) : ASimpleServer(source)
{
	*this = source;
	std::cout << GRN "the TestServer ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

TestServer::~TestServer(void)
{
	std::cout << GRN "the TestServer ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

TestServer &TestServer::operator=(TestServer const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, TestServer const &source)
{
	(void)source;
	out << BLU "TestServer";
	out << DEF << std::endl;
	return (out);
}

void TestServer::removeClient(int fd, size_t& index)
{
    _clients.erase(fd);
    close(fd);
    _pollfds.erase(_pollfds.begin() + index);
    index--;
}
