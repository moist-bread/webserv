#include "../../inc/sockets/ASimpleServer.hpp"
#include "../../inc/ansi_color_codes.h"

ASimpleServer::ASimpleServer(int domain,int type,int protocol,int port,u_long ip,int backlog) : ListeningSocket(domain,type,protocol,port,ip,backlog)
{
	std::cout << GRN "the ASimpleServer ";
	std::cout << UCYN "has been created" DEF << std::endl;
    this->_backlog = backlog;
}

ASimpleServer::ASimpleServer(ASimpleServer const &source) : ListeningSocket(source)
{
	*this = source;
	std::cout << GRN "the ASimpleServer ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

ASimpleServer::~ASimpleServer(void)
{
	std::cout << GRN "the ASimpleServer ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

ListeningSocket *ASimpleServer::getSocket() const{
    return this->_socket;
}

ASimpleServer &ASimpleServer::operator=(ASimpleServer const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, ASimpleServer const &source)
{
	(void)source;
	out << BLU "ASimpleServer";
	out << DEF << std::endl;
	return (out);
}
