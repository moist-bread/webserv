#include "../../inc/requests/HTTP.hpp"

const std::string HTTP::_method_names[] = {"GET", "POST", "PUT", "DELETE", "PATCH"};
const std::string HTTP::_protocol_names[] = {"HTTP/1.0", "HTTP/1.1"};

HTTP::HTTP(void)
{
	std::cout << GRN "the HTTP ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

HTTP::HTTP(HTTP const &source)
{
	*this = source;
	std::cout << GRN "the HTTP ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

HTTP::~HTTP(void)
{
	std::cout << GRN "the HTTP ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

HTTP &HTTP::operator=(HTTP const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

t_method HTTP::getMethod(const std::string &strMethod)
{
	for (size_t i = 0; i < _method_names->size();i++)
	{
		if (!strMethod.compare(_method_names[i]))
			return (static_cast<t_method>(i));
	}
	return (UNSUPPORTED_METHOD);
}

std::string HTTP::stringMethod(const t_method Method)
{
	return (_method_names[Method]);
}

t_protocol HTTP::getProtocol(const std::string &strProtocol)
{
	for (size_t i = 0; i < _method_names->size();i++)
	{
		if (!strProtocol.compare(_protocol_names[i]))
			return (static_cast<t_protocol>(i));
	}
	return (UNSUPPORTED_PROTOCOL);
}

std::string HTTP::stringProtocol(const t_protocol Protocol)
{
	return (_protocol_names[Protocol]);
}

t_status_code HTTP::getStatusCode()
{
	return (OK);
}

