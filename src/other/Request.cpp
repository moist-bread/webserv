#include "../../inc/requests/Request.hpp"

std::string method_names[] = {"GET", "POST", "PUT", "DELETE", "PATCH", ""};
std::string protocol_names[] = {"HTTP/1.0", "HTTP/1.1", ""};

Request::Request(void) : method(UNSUPPORTED_METHOD), protocol(UNSUPPORTED_PROTOCOL)
{
	std::cout << GRN "the Request ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Request::Request(char *rec) : method(UNSUPPORTED_METHOD), protocol(UNSUPPORTED_PROTOCOL)
{
	std::string request = rec;
	size_t len;

	// -- GET METHOD
	this->method = static_cast<t_method>(segment_compare_extract(&request, (std::string) " ", method_names));

	// -- GET URI
	len = request.find(" ");
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request"));
	this->path_uri = request.substr(0, len);
	request.erase(0, len + 1);

	// -- GET PROTOCOL
	this->protocol = static_cast<t_protocol>(segment_compare_extract(&request, (std::string)"\r\n", protocol_names));
	// this->protocol = static_cast<t_protocol>(segment_compare_extract(&request, (std::string) "\n", protocol_names)); // testing with text

	// -- GET HEADERS
	std::string key;
	std::string value;
	while (!request.empty())
	{
		len = request.find(": ");
		if (len == std::string::npos)
		{
			if (request.find_first_not_of("\r\n") != std::string::npos)
				throw(Request::ParseError("Miss formated request"));
			else
				break;
		}
		key = request.substr(0, len);
		request.erase(0, len + 2);
		len = request.find("\r\n");
		// len = request.find("\n"); //  testing with text
		if (len == std::string::npos)
			len = request.size();
		value = request.substr(0, len);
		request.erase(0, len + 2);
		headers[key] = value;
	}

	std::cout << *this;
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
	{
		this->method = source.method;
		this->path_uri = source.path_uri;
		this->protocol = source.protocol;
		this->headers = source.headers;
	}
	return (*this);
}

int Request::segment_compare_extract(std::string *src, std::string sep, std::string *cmp) const
{
	size_t len = (*src).find(sep);
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request"));

	std::string segment = (*src).substr(0, len);
	(*src).erase(0, len + sep.length());

	for (int i = 0; !cmp[i].empty(); i++)
		if (!segment.compare(cmp[i]))
			return (i);
	throw(Request::ParseError("Unsupported parameter"));
}

std::ostream &operator<<(std::ostream &out, Request &source)
{
	out << BLU "-- Request Information --";
	out << DEF << std::endl << std::endl;
	out << BLU "Method: " DEF << method_names[source.method] << std::endl;
	out << BLU "URI: " DEF << source.path_uri << std::endl;
	out << BLU "PROTOCOL: " DEF << protocol_names[source.protocol] << std::endl;
	out << BLU "Headers..." DEF << std::endl;
	for (std::map<std::string, std::string>::iterator it = source.headers.begin(); it != source.headers.end(); it++)
		out << BLU "    [" << (*it).first << "]" DEF "  " << (*it).second << std::endl;
	out << std::endl;
	return (out);
}
