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

void TestServer::responder(int clientFd)
{
    const char *hello = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nHello from server";
    write(clientFd, hello, strlen(hello));
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
		//Sou a socket principal e estou a receber uma nova ligacao
		if(fd == this->_sock)
		{
			//OPA pera la que temos uma socket que esta em modo input
			if(_pollfds[i].revents & POLLIN)
			{
				//Vamos aceita-la e coloca la no vector
				accepter();
			}
		}else
		{

			/*
				Cliente esta a enviar um request
			*/

			//Sou uma socket existente cliente
			//Se tiver algum cliente em modo input altura de agir
			if(_pollfds[i].revents & POLLIN)
			{
				char tmp[1024];
				int ret = recv(_pollfds[i].fd,tmp,sizeof(tmp),0);

				switch (getStatus(ret))
				{
				case IO_ERROR:
					std::cout << "Comeback Later something went wrong" << std::endl;
					removeClient(fd, i);
					break;
				case IO_CLOSED:
					std::cout << "Timeout...please try again" << std::endl;
					removeClient(fd, i);
					break;
				case IO_DATA_READY:
					//Vamos guardar no buffer			
					_clients[fd].feed(tmp,ret);


					std::cout << "O cliente " << fd << " tem " 
							  << _clients[fd].GetRequestBuffer().size() << " bytes acumulados." << std::endl;



					//Printar output
					 handler(_clients[fd].GetRequestBuffer());
					std::cout << _clients[fd].GetRequestBuffer() << std::endl;
					responder(fd);


					//Verificar se o request acabou

					if(_clients[fd].IsRequestDone())
					{
						std::cout << "Yupii" << std::endl;
					}

					break;

					case IO_DATA_OUT:
					/*
						Sou o servidor e vou enviar uma resposta
					*/
					std::cout << "wawawaw" << std::endl;
					break;
				}
			}
		}

	}		
		std::cout << "== DONE ===" << std::endl;
	}
	
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
