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
	method = static_cast<t_method>(extract_cmp_verify(&request, " ", method_names));

	// -- GET URI
	len = request.find(" ");
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request"));
	path_uri = request.substr(0, len);
	request.erase(0, len + 1);

	// -- GET QUERY
	len = path_uri.find("?");
	if(len != std::string::npos)
	{
		// -- extract query
		std::string remaining_query;
		remaining_query = path_uri.substr(len + 1, path_uri.length() - len);
		path_uri.erase(len, path_uri.length() - len);
		query = extract_key_value(&remaining_query, "=", "&");
	}

	// -- GET PROTOCOL
	protocol = static_cast<t_protocol>(extract_cmp_verify(&request, CRLF, protocol_names));
	// protocol = static_cast<t_protocol>(extract_cmp_verify(&request, "\n", protocol_names)); // testing with text

	// -- GET HEADERS
	headers = extract_key_value(&request, ":", CRLF);
	// headers = extract_key_value(&request, ":", "\n"); //  testing with text

	len = request.find_first_not_of(CRLF);
	if (len != std::string::npos)
		request.erase(0, len);
	
	// -- GET BODY
	body = extract_key_value(&request, "=", "&");

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

int Request::extract_cmp_verify(std::string *src, const char *sep, std::string *cmp) const
{
	size_t len = (*src).find(sep);
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request"));

	std::string segment = (*src).substr(0, len);
	(*src).erase(0, len + (static_cast<std::string>(sep)).length());

	for (int i = 0; !cmp[i].empty(); i++)
		if (!segment.compare(cmp[i]))
			return (i);
	throw(Request::ParseError("Unsupported parameter"));
}

map_strings Request::extract_key_value(std::string *src, std::string sep, std::string delim) const
{
	map_strings map;
	std::string key;
	std::string value;
	size_t len;
	size_t lws;

	while (!(*src).empty())
	{
		// std::cout << "-- remaining src: " << std::endl << (*src) << std::endl;
		
		// -- get the key name
		len = (*src).find(sep);
		if (len == std::string::npos)
			break;
		key = (*src).substr(0, len);
		(*src).erase(0, len + sep.length());

		// -- get the value content
		len = (*src).find(delim);
		if (len == std::string::npos)
			len = (*src).size();
		lws = (*src).find_first_not_of(" \t\n\v\f\r"); // verify lws better
		if (lws == std::string::npos)
			lws = 0;
		value = (*src).substr(lws, len - lws);
		(*src).erase(0, len + delim.length());
		map[key] = value;
	}
	return (map);
}

std::ostream &operator<<(std::ostream &out, Request &source)
{
	out << BLU "-- Request Information --";
	out << DEF << std::endl << std::endl;
	out << BLU "Method: " DEF << method_names[source.method] << std::endl;
	out << BLU "URI: " DEF << source.path_uri << std::endl;
	if (!source.query.empty())
	{
		out << BLU "    Query..." DEF << std::endl;
		for (map_strings::iterator it = source.query.begin(); it != source.query.end(); it++)
			out << BLU "        [" << (*it).first << "]" DEF " |" << (*it).second << "|"<< std::endl;
	}
	out << BLU "PROTOCOL: " DEF << protocol_names[source.protocol] << std::endl;
	out << BLU "Headers..." DEF << std::endl;
	for (map_strings::iterator it = source.headers.begin(); it != source.headers.end(); it++)
		out << BLU "    [" << (*it).first << "]" DEF " |" << (*it).second << "|"<< std::endl;
	if (!source.body.empty())
	{
		out << BLU "Body..." DEF << std::endl;
		for (map_strings::iterator it = source.body.begin(); it != source.body.end(); it++)
			out << BLU "    [" << (*it).first << "]" DEF " |" << (*it).second << "|"<< std::endl;
		out << std::endl;
	}
	return (out);
}
