#include "../../inc/sockets/TestServer.hpp"

TestServer::TestServer(void) : ASimpleServer(AF_INET,SOCK_STREAM,0,8080,INADDR_ANY,10)
{
	std::memset(_buffer, 0, sizeof(_buffer));
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
    this->_newSocket = accept(this->_sock,(struct sockaddr*)&this->_address,(socklen_t *)&addrlen);
    read(this->_newSocket,this->_buffer,30000);
}

void TestServer::handler()
{
    std::cout << _buffer << std::endl;
}

void TestServer::responder()
{
    const char *hello = "Hello from server";
    write(this->_newSocket,hello,17);
    close(_newSocket);
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

void TestServer::launch()
{
	SetNonblocking(this->_sock);
	//Hmmmm meio que sem o control + c nao tenho maneira de parar o loop 
	//signalHandler();
    while (true)
	{
		std::cout << "=== WAITING ===" << std::endl;
		accepter();
		handler();
		responder();
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
