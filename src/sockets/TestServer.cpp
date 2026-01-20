#include "TestServer.hpp"


TestServer::TestServer(void) : ASimpleServer()
{
	std::cout << GRN "the TestServer ";
	std::cout << UCYN "has been created" DEF << std::endl;
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
    char *hello = "Hello from server";
    write(this->_newSocket,hello,strlen(hello));
    close(_newSocket);
}

void TestServer::launch()
{
    
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
