#include "../../inc/requests/Request.hpp"
#include "../../inc/requests/Response.hpp"

std::string method_names[] = {"GET", "POST", "PUT", "DELETE", "PATCH", ""};
std::string protocol_names[] = {"HTTP/1.0", "HTTP/1.1", ""};

Request::Request(void) : method(UNSUPPORTED_METHOD), protocol(UNSUPPORTED_PROTOCOL)
{
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
		this->query = source.query;
		this->file_extension = source.file_extension;
		this->protocol = source.protocol;
		this->headers = source.headers;
		this->body = source.body;
	}
	return (*this);
}

void Request::process(std::string request)
{
	size_t len;

	clear();
	// -- GET METHOD
	method = static_cast<t_method>(extract_cmp_verify(&request, " ", method_names));

	// -- GET URI
	len = request.find(" ");
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request", BAD_REQUEST));
	path_uri = request.substr(0, len);
	request.erase(0, len + 1);

	// -- GET QUERY
	len = path_uri.find("?");
	if (len != std::string::npos)
	{
		query = path_uri.substr(len + 1, path_uri.length() - len);
		path_uri.erase(len, path_uri.length() - len);
	}

	// -- GET FILE EXTENSION
	len = path_uri.rfind(".");
	if (len != std::string::npos)
	{
		file_extension = path_uri.substr(len + 1, path_uri.length() - len);
		len = file_extension.find("/");
		if (len != std::string::npos)
			file_extension.erase(len, file_extension.length() - len);
	}
	else
		file_extension = "html";

	// -- GET PROTOCOL
	protocol = static_cast<t_protocol>(extract_cmp_verify(&request, CRLF, protocol_names));
	// protocol = static_cast<t_protocol>(extract_cmp_verify(&request, "\n", protocol_names)); // testing with text

	// -- GET HEADERS
	headers = extract_key_value(&request, ":", CRLF);
	// headers = extract_key_value(&request, ":", "\n"); // testing with text

	// -- GET BODY
	request.erase(0, 2);
	// request.erase(0, 1); // testing with text

	if (headers["content-length"].empty() && request.size())
		throw(Request::ParseError("Missing required headers", LENGTH_REQUIRED));
	else if (!headers["content-length"].empty())
	{
		char *end = NULL;
		double value = std::strtod(headers["content-length"].c_str(), &end);
		if (*end || value == HUGE_VAL || value == -HUGE_VAL || value != request.size())
			throw(Request::ParseError("Incorrect Content Length", BAD_REQUEST));
	}
	body = request.substr(0, request.size());

	// !! check if method is valid for the locations in config !!
	// throw(Request::ParseError("Invalid method \"" + method_names[method] + "\"" , METHOD_NOT_ALLOWED));

	std::cout << *this << std::endl;
}

void Request::clear(void)
{
	method = UNSUPPORTED_METHOD;
	path_uri.clear();
	query.clear();
	file_extension.clear();
	protocol = UNSUPPORTED_PROTOCOL;
	headers.clear();
	body.clear();
}

int Request::extract_cmp_verify(std::string *src, const char *sep, std::string *cmp) const
{
	size_t len = (*src).find(sep);
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request", BAD_REQUEST));

	std::string segment = (*src).substr(0, len);
	(*src).erase(0, len + (static_cast<std::string>(sep)).length());

	for (int i = 0; !cmp[i].empty(); i++)
		if (!segment.compare(cmp[i]))
			return (i);
	throw(Request::ParseError("Unsupported parameter", BAD_REQUEST));
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
		// -- get the key name
		len = (*src).find(sep);
		if (len == std::string::npos)
			break;
		key = (*src).substr(0, len);
		// a header is a case-insensitive name
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		(*src).erase(0, len + sep.length());

		// -- get the value content
		len = (*src).find(delim);
		if (len == std::string::npos)
			len = (*src).size();
		lws = (*src).find_first_not_of(" \t\n\v\f\r"); // !!!!!! verify lws better
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
	out << DEF << std::endl
		<< std::endl;
	out << BLU "Method: " DEF << method_names[source.method] << std::endl;
	out << BLU "URI: " DEF << source.path_uri << std::endl;
	out << BLU "file extension... " DEF << source.file_extension << std::endl;
	if (!source.query.empty())
	{
		out << BLU "    Query..." DEF << std::endl;
		out << BLU "        [" << source.query << "]" DEF << std::endl;
	}
	out << BLU "PROTOCOL: " DEF << protocol_names[source.protocol] << std::endl;
	out << BLU "Headers..." DEF << std::endl;
	for (map_strings::iterator it = source.headers.begin(); it != source.headers.end(); it++)
		out << BLU "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;
	if (!source.body.empty())
	{
		out << BLU "Body..." DEF << std::endl;
		out << BLU "        [" << source.body << "]" DEF << std::endl;
		out << std::endl;
	}
	return (out);
}
