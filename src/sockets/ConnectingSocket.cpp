#include "../../inc/sockets/ConnectingSocket.hpp"
#include "../../inc/ansi_color_codes.h"

ConnectingSocket::ConnectingSocket(int domain,int type,int protocol,int port,u_long ip): SocketController(domain,type,protocol,port,ip)
{
	std::cout << GRN "the ConnectingSocket ";
	std::cout << UCYN "has been created" DEF << std::endl;
    
	this->_connectSocket = connect_to_network(getSocketfd(),getStructAdress());
	test_connection(_connectSocket);
}

ConnectingSocket::ConnectingSocket(ConnectingSocket const &source) : SocketController(source)
{
	*this = source;
	std::cout << GRN "the ConnectingSocket ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

ConnectingSocket &ConnectingSocket::operator=(ConnectingSocket const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

int ConnectingSocket::connect_to_network(int sock,struct sockaddr_in address)
{
    return connect(sock,(struct sockaddr *)&address,sizeof(address));
} 

std::ostream &operator<<(std::ostream &out, ConnectingSocket const &source)
{
	(void)source;
	out << BLU "ConnectingSocket";
	out << DEF << std::endl;
	return (out);
}

ConnectingSocket::~ConnectingSocket(void)
{
	std::cout << GRN "the ConnectingSocket ";
	std::cout << URED "has been deleted" DEF << std::endl;
}
