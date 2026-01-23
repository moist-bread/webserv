#include "../../inc/sockets/BindingSocket.hpp"
#include "../../inc/ansi_color_codes.h"


BindingSocket::BindingSocket(int domain,int type,int protocol,int port,u_long ip): SocketController(domain,type,protocol,port,ip)
{
	std::cout << GRN "the BindingSocket ";
	std::cout << UCYN "has been created" DEF << std::endl;
    
	this->_binding = connect_to_network(getSocketfd(),getStructAdress());
	test_connection(_binding);
}

BindingSocket::BindingSocket(BindingSocket const &source) : SocketController(source)
{
	*this = source;
	std::cout << GRN "the BindingSocket ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}



BindingSocket &BindingSocket::operator=(BindingSocket const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}
int BindingSocket::connect_to_network(int sock,struct sockaddr_in address)
{
    return bind(sock,(struct sockaddr *)&address,sizeof(address));
} 

std::ostream &operator<<(std::ostream &out, BindingSocket const &source)
{
	(void)source;
	out << BLU "BindingSocket";
	out << DEF << std::endl;
	return (out);
}

BindingSocket::~BindingSocket(void)
{
	std::cout << GRN "the BindingSocket ";
	std::cout << URED "has been deleted" DEF << std::endl;
}