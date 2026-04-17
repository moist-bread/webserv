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

	if (status_code != OK)
		src.method = GET;
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
	// create a special default location path to dump all POST info into
	// open the file and write to it 

	// -- later get "www" replacement from config
	std::string path = "www" + src.path_uri + "database";
	std::ofstream output (path.c_str() , std::ofstream::out | std::ostream::app);

	if (!output.is_open())
	{
		status_code = INTERNAL_SERVER_ERROR;
		method_get(src);
	}
	else
	{
		if (!src.json.empty())
			handle_application_form(src);
		else if (!src.multi_form.empty())
			handle_multipart_form(src);
		else
		{
			body = src.body;
			src.file_extension = "html";
		}
		
		output << body ;
		output.close();
		headers["Location"] = src.path_uri; 
	}
}

void Response::handle_application_form(Request &src)
{
	// json to the body...
	body = src.json + CRLF;
	src.file_extension = "json";

	// FOUND will make the client request a GET for a redirection location 
	status_code = FOUND;
}
void Response::handle_multipart_form(Request &src)
{
	for (std::vector<MultiForm>::iterator it = src.multi_form.begin(); it != src.multi_form.end(); it++)
	{
		map_strings::iterator f_name = (*it).content_disposition.find("filename");
		if (f_name != (*it).content_disposition.end())
		{
			// Use a random or unique naming strategy for uploaded files to avoid overwriting existing files.
			// generate random name
			std::string path = "www" + src.path_uri + (*f_name).second;

			// create a file in a files directory inside of where we are
			// maybe make directory names based on (*it).content_type or on (*it).content_disposition.find("name")
			std::ofstream output (path.c_str(), std::ofstream::out);
			if (!output.is_open())
			{
				status_code = INTERNAL_SERVER_ERROR;
				method_get(src);
			}
			
			// put the (*it).data in the file
			output << (*it).data << CRLF;
			output.close();
		}
		else
		{
			// save it into the "database" file as i would for json??
			body += (*it).data + CRLF;
		}
	}
	src.file_extension = "html";

	status_code = FOUND;
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
	else if (extension == "mp4")
		return ("video/mp4");
	else if (extension == "webm")
		return ("video/webm");
	else if (extension == "mpeg")
		return ("audio/mpeg");
	else if (extension == "wav")
		return ("audio/wav");

	// app types
	else if (extension == "pdf")
		return ("application/pdf");
	else if (extension == "form") // --------------------
		return ("application/x-www-form-urlencoded");
	else if (extension == "json") // --------------------
		return ("application/json");

	// multipart types
	else if (extension == "multiform") // --------------------
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
	out << YEL "Body..." DEF << std::endl;
	if (source.headers["Content-Type"] == "image/vnd.microsoft.icon")
		out << "[PAGE ICON IMAGE]" << std::endl;
	else
		out << source.body << std::endl;
	/*
	if (!source.full_response.empty())
		out << YEL "Full Response..." DEF << std::endl << source.full_response << std::endl;
	*/
	out << std::endl;
	return (out);
}
