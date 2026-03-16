#include "../../inc/requests/Response.hpp"

extern std::string protocol_names[];

Response::Response(void) : protocol(UNSUPPORTED_PROTOCOL), status_code(OK)
{
	std::cout << GRN "the Response ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Response::Response(Response const &source)
{
	*this = source;
	std::cout << GRN "the Response ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Response::~Response(void)
{
	std::cout << GRN "the Response ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Response &Response::operator=(Response const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
	{
		this->protocol = source.protocol;
		this->status_code = source.status_code;
		this->headers = source.headers;
		this->body = source.body;
		this->full_response = source.full_response;
	}
	return (*this);
}

void Response::process(Request &src)
{
	update_response_elements(src);

	// get the requested content
	std::ifstream file(assemble_content_path(src, status_code).c_str());
	if(!file.is_open())
	{
		perror("File does not exist");
		if (src.path_uri.find(".html") == src.path_uri.size() - 5)
			status_code = NOT_FOUND;
		else
			status_code = INTERNAL_SERVER_ERROR;
		file.open(assemble_content_path(src, status_code).c_str());
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	if(content.empty())
	{
		perror("Desired error page is not present");
		status_code = INTERNAL_SERVER_ERROR;
	}
	
	// putting together the full response
	std::stringstream ss;
	ss << protocol_names[protocol] << " " << status_code << " " << get_reason_phrase(status_code) << CRLF;
	ss << "Content-Length: " << content.size() << CRLF;
	// -- Content-Type
	ss << "Date: " << date_generate() << CRLF;
	ss << CRLF;
	ss << content;
	full_response = ss.str();

	std::cout << GRN "New Response ";
	std::cout << UYEL "has been processed" DEF << std::endl;
	std::cout << *this << std::endl;
}

void Response::update_response_elements(Request &src)
{
	protocol = src.protocol;
	if (protocol_names[protocol].empty())
		protocol = H1_0;
	headers.clear();
	body.clear();
	full_response.clear();
}
std::string Response::assemble_content_path(Request &src, t_status_code status_code)
{
	std::string path;
	if (status_code != OK)
	{
		// in case of error
		// look at the config to see which error page to send
		// (later) consider default error pages from config
		path = "www/404.html";
	}
	else
	{
		// in case of everything being fine
		// do path with
		// (later) consider locations from config
		if (src.path_uri.find(".") == std::string::npos)
			src.path_uri.append("index.html");
		path = "www" + src.path_uri;
	}
	return (path);
}

std::string Response::get_reason_phrase(t_status_code status_code)
{
	switch (status_code)
	{
		case CONTINUE:
			return ("Continue");
		case SWITCHING_PROTOCOL:
			return ("Switching Protocols");
		case OK:
			return ("OK");
		case CREATED:
			return ("Created");
		case ACCEPTED:
			return ("Accepted");
		case NON_AUTHORITATIVE_INFO:
			return ("Non-authoritative Information");
		case NO_CONTENT:
			return ("No Content");
		case RESET_CONTENT:
			return ("Reset Content");
		case PARTIAL_CONTENT:
			return ("Partial Content");
		case MULTIPLE_CHOICES:
			return ("Multiple Choices");
		case MOVED_PERMANENTLY:
			return ("Moved Permanently");
		case FOUND:
			return ("Found");
		case SEE_OTHER:
			return ("See Other");
		case NOT_MODIFIED:
			return ("Not Modified");
		case TEMPORARY_REDIRECT:
			return ("Temporary Redirect");
		case PERMANENT_REDIRECT:
			return ("Permanent Redirect");
		case BAD_REQUEST:
			return ("Bad Request");
		case UNAUTHORIZED:
			return ("Unauthorized");
		case PAYMENT_REQUIRED:
			return ("Payment Required");
		case FORBIDDEN:
			return ("Forbidden");
		case NOT_FOUND:
			return ("Not Found");
		case METHOD_NOT_ALLOWED:
			return ("Method Not Allowed");
		case NOT_ACCEPTABLE:
			return ("Not Acceptable");
		case PROXY_AUTHENTICATION_REQUIRED:
			return ("Proxy Authentication Required");
		case REQUEST_TIMEOUT:
			return ("Request Timeout");
		case CONFLICT:
			return ("Conflict");
		case GONE:
			return ("Gone");
		case LENGTH_REQUIRED:
			return ("Length Required");
		case PRECONDITION_FAILED:
			return ("Precondition Failed");
		case REQUEST_ENTITY_TOO_LARGE:
			return ("Request Entity Too Large");
		case REQUEST_URL_TOO_LONG:
			return ("Request-url Too Long");
		case UNSUPPORTED_MEDIA_TYPE:
			return ("Unsupported Media Type");
		case REQUESTED_RANGE_NOT_SATISFIABLE:
			return ("Requested Range Not Satisfiable");
		case EXPECTATION_FAILED:
			return ("Expectation Failed");
		case INTERNAL_SERVER_ERROR:
			return ("Internal Server Error");
		case NOT_IMPLEMENTED:
			return ("Not Implemented");
		case BAD_GATEWAY:
			return ("Bad Gateway");
		case SERVICE_UNAVAILABLE:
			return ("Service Unavailable");
		case GATEWAY_TIMEOUT:
			return ("Gateway Timeout");
		case HTTP_VERSION_NOT_SUPPORTED:
			return ("HTTP Version Not Supported");
		default:
			return ("OK");
	}
}

std::string	Response::date_generate(void)
{
	time_t timestamp = time(NULL);
	struct tm datetime = *localtime(&timestamp);
	char buff[30];
	std::string buffer;

	// example: Fri, 13 Mar 2026 17:32:50 GMT
	if (strftime(buff, sizeof buff, "%a, %d %b %Y %T GMT", &datetime))
		buffer = buff;
	return (buffer);
}


std::ostream &operator<<(std::ostream &out, Response &source)
{
	out << YEL "-- Response Information --";
	out << DEF << std::endl << std::endl;
	out << YEL "PROTOCOL: " DEF << protocol_names[source.protocol] << std::endl;
	out << YEL "Status Code: " DEF << source.status_code << std::endl;
	out << YEL "Reason Phrase: " DEF << source.get_reason_phrase(source.status_code) << std::endl;
	out << YEL "Headers..." DEF << std::endl;
	for (map_strings::iterator it = source.headers.begin(); it != source.headers.end(); it++)
		out << YEL "    [" << (*it).first << "]" DEF " |" << (*it).second << "|"<< std::endl;
	out << YEL "Body..." DEF << std::endl << source.body << std::endl;
	if (!source.full_response.empty())
		out << YEL "Full Response..." DEF << std::endl << source.full_response << std::endl;
	out << std::endl;
	return (out);
}
