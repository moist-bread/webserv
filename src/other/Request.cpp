#include "../../inc/requests/Request.hpp"

Request::Request(void)
{
	std::cout << GRN "the Request ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Request::Request(char *rec)
{
	(void)rec;
	std::cout << GRN "the Request ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Request::Request(Request const &source)
{
	*this = source;
	std::cout << GRN "the Request ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Request::~Request(void)
{
	std::cout << GRN "the Request ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Request &Request::operator=(Request const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, Request const &source)
{
	(void)source;
	out << BLU "Request";
	out << DEF << std::endl;
	return (out);
}
