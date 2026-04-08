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
	clear(src);

	switch (src.method)
	{
	case GET:
		method_get(src);
		break;
	case POST:
		method_post(src);
		break;
	default:
		method_get(src);
		break;
	}
	
	headers["Content-Length"] = to_str(body.size());
	headers["Content-Type"] = define_content_type(src.file_extension);
	headers["Connection"] = src.headers["connection"];
	headers["Date"] = date_generate();
		
	// putting together the full response
	std::stringstream ss;
	ss << protocol_names[protocol] << " " << status_code << " " << get_reason_phrase(status_code) << CRLF;
	for (map_strings::iterator it = headers.begin(); it != headers.end(); it++)
		ss << (*it).first << ": " << (*it).second << CRLF;
	ss << CRLF;
	ss << body;
	full_response = ss.str();

	std::cout << GRN "New Response ";
	std::cout << UYEL "has been processed" DEF << std::endl;
	std::cout << *this << std::endl;
}

void Response::clear(Request &src)
{
	protocol = src.protocol;
	if (protocol_names[protocol].empty())
		protocol = H1_0;
	headers.clear();
	body.clear();
	full_response.clear();
}

void Response::method_get(Request &src)
{
	// get the requested content
	std::ifstream file(assemble_content_path(src, status_code).c_str());
	if (!file.is_open())
	{
		perror("File does not exist");
		if (src.file_extension == "html")
			status_code = NOT_FOUND;
		else
			status_code = INTERNAL_SERVER_ERROR;
		src.file_extension = "html";
		file.open(assemble_content_path(src, status_code).c_str());
	}

	if (file.is_open())
		body = to_str(file.rdbuf());
	else
		body = backup_error_pages(status_code);
}

void Response::method_post(Request &src)
{
	std::cout << "THIS IS A POST METHOD\n";

	// create a special default location path to dump all PUT information into
	// open the file and write to it 

	method_get(src);
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
		src.file_extension = "html";
	}
	else
	{
		// in case of everything being fine
		// do path with
		// (later) consider locations from config
		if (src.file_extension == "html")
		{
			if (!(*src.path_uri.end() == '/'))
				src.path_uri.append("/");
			src.path_uri.append("index.html");
		}
		path = "www" + src.path_uri;
	}
	return (path);
}
std::string Response::backup_error_pages(t_status_code status_code)
{
	std::stringstream ss;
	ss << "<!DOCTYPE html>" << std::endl;
	ss << "<html lang=\"en\">" << std::endl;
	ss << "<head>" << std::endl;
	ss << "	<meta charset=\"UTF-8\">" << std::endl;
	ss << "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << std::endl;
	ss << "	<title>" << status_code << " " << get_reason_phrase(status_code) << "</title>" << std::endl;
    ss << " <link rel=\"stylesheet\" href=\"/index.css\" type=\"text/css\">" << std::endl;
	ss << "</head>" << std::endl;
	ss << "<body>" << std::endl;
	ss << "<div class=\"text yellow-mark\"> Error: " << status_code << " " << get_reason_phrase(status_code) << "</div>" << std::endl;
	ss << "</body>" << std::endl;
	ss << "</html>" << std::endl;
	return (ss.str());
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

const char *Response::define_content_type(std::string extension) const
{
	// text types
	if (extension == "html")
		return ("text/html");
	else if (extension == "css")
		return ("text/css");
	else if (extension == "csv")
		return ("text/csv");
	else if (extension == "txt")
		return ("text/plain");

	// image types
	else if (extension == ".jpeg" || extension == ".jpg")
		return ("image/jpeg");
	else if (extension == "png")
		return ("image/png");
	else if (extension == "gif")
		return ("image/gif");
	else if (extension == "svg")
		return ("image/svg+xml");
	else if (extension == "webp")
		return ("image/webp");
	else if (extension == "ico")
		return ("image/vnd.microsoft.icon");

	// video and audio types
	if (extension == "mp4")
		return ("video/mp4");
	else if (extension == "webm")
		return ("video/webm");
	else if (extension == "mpeg")
		return ("audio/mpeg");
	else if (extension == "wav")
		return ("audio/wav");

	// app types
	if (extension == "pdf")
		return ("application/pdf");
	else if (extension == "form") // --------------------
		return ("application/x-www-form-urlencoded");

	// multipart types
	if (extension == "multiform") // --------------------
		return ("multipart/form-data");
	else if (extension == "byteranges") // --------------------
		return ("multipart/byteranges");

	else
		return ("application/octet-stream");
}

std::string Response::date_generate(void)
{
	time_t timestamp = std::time(NULL);
	struct tm datetime = *std::localtime(&timestamp);
	char buff[30];
	std::string buffer;

	// example: Fri, 13 Mar 2026 17:32:50 GMT
	if (std::strftime(buff, sizeof buff, "%a, %d %b %Y %T GMT", &datetime))
		buffer = buff;
	return (buffer);
}

void Response::eraseWritten(int start, int idx)
{
	if (start >= 0 && start + idx <= (int)full_response.size())
	{
		full_response.erase(start, idx);
	}
}

std::ostream &operator<<(std::ostream &out, Response &source)
{
	out << YEL "-- Response Information --";
	out << DEF << std::endl
		<< std::endl;
	out << YEL "PROTOCOL: " DEF << protocol_names[source.protocol] << std::endl;
	out << YEL "Status Code: " DEF << source.status_code << std::endl;
	out << YEL "Reason Phrase: " DEF << source.get_reason_phrase(source.status_code) << std::endl;
	out << YEL "Headers..." DEF << std::endl;
	for (map_strings::iterator it = source.headers.begin(); it != source.headers.end(); it++)
		out << YEL "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;
	out << YEL "Body..." DEF << std::endl
		<< source.body << std::endl;
	/*
	if (!source.full_response.empty())
		out << YEL "Full Response..." DEF << std::endl << source.full_response << std::endl;
	*/
	out << std::endl;
	return (out);
}
