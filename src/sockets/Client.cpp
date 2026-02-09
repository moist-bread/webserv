#include "../../inc/sockets/Client.hpp"


Client::Client(void)
{
	std::cout << GRN "the Client ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Client::Client(Client const &source)
{
	*this = source;
	std::cout << GRN "the Client ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Client::~Client(void)
{
	std::cout << GRN "the Client ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Client &Client::operator=(Client const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, Client const &source)
{
	(void)source;
	out << BLU "Client";
	out << DEF << std::endl;
	return (out);
}
