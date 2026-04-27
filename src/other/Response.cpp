#include "../../inc/requests/Response.hpp"
#include "../../inc/requests/Request.hpp"

#include "../../inc/ansi_color_codes.h"

#include <ctime>		// time, localtime, strftime
#include <fstream>		// remove, fstream, ifstream, ofstream
#include <dirent.h>		// opendir, readdir, closedir, DIR
#include <stdlib.h>		// rand
#include <sys/stat.h>	// mkdir, stat

Response::Response(void) : protocol(UNSUPPORTED_PROTOCOL), status_code(OK) {}

Response::Response(Response const &source)
{
	*this = source;
}

Response::~Response(void) {}

Response &Response::operator=(Response const &source)
{
	if (this != &source)
	{
		this->protocol = source.protocol;
		this->status_code = source.status_code;
		this->headers = source.headers;
		this->body = source.body;
		this->cgi_reply = source.cgi_reply;
		this->full_response = source.full_response;
	}
	return (*this);
}

void Response::process(Request &src)
{
	if (!cgi_reply.empty())
	{
		std::string tmp = cgi_reply;
		clear(src);
		full_response = tmp;
		return;
	}
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
	case DELETE:
		method_delete(src);
		break;
	default:
		status_code = METHOD_NOT_ALLOWED;
		method_get(src);
		break;
	}

	headers["Content-Length"] = to_str(body.size());
	headers["Content-Type"] = define_content_type(src.file_extension);
	headers["Connection"] = src.headers["connection"];
	headers["Date"] = date_generate();

	// putting together the full response
	std::stringstream ss;
	ss << HTTP::stringProtocol(src.protocol) << " " << status_code << " " << HTTP::getReasonPhrase(status_code) << CRLF;
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
	if (protocol == UNSUPPORTED_PROTOCOL)
		protocol = H1_0;
	headers.clear();
	body.clear();
	cgi_reply.clear();
	full_response.clear();
}

void Response::method_get(Request &src)
{
	// get the requested content
	if (src.path_uri == "/uploads") // IN CASE OF AUTO INDEXING
	{
		body = create_autoindexing_page(src);
		return;
	}
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
		body = backup_error_page(status_code);
}

std::string Response::create_autoindexing_page(Request &src)
{
	DIR *d = opendir(("www" + src.path_uri).c_str());
	struct dirent *elem;
	if (!d)
	{
		status_code = NOT_FOUND;
		return (backup_error_page(status_code));
	}

	std::stringstream ss;
	ss << "<!DOCTYPE html>" << std::endl;
	ss << "<html lang=\"en\">" << std::endl;
	ss << "<head>" << std::endl;
	ss << "	<meta charset=\"UTF-8\">" << std::endl;
	ss << "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << std::endl;
	ss << "	<title>listing directory www/" << src.path_uri << "</title>" << std::endl; // -- CHANGE WWW TO NEW CONFIG
	ss << " 	<link rel=\"stylesheet\" href=\"/index.css\" type=\"text/css\">" << std::endl;
	ss << "	</head>" << std::endl;
	ss << "	<body>" << std::endl;

	ss << "	<div id=\"wrapper\">" << std::endl;

	ss << "		<h1>";
	ss << "<a href=\"" << "http://localhost:8080/" << "\">~</a> / "; // -- CHANGE locahost TO NEW CONFIG
	std::string folders = src.path_uri;
	folders.erase(0, 1);
	while (!folders.empty())
	{
		size_t pin = folders.find("/");
		std::cout << RED << folders << std::endl;
		if (pin != std::string::npos) // -- CHANGE locahost TO NEW CONFIG
			ss << "<a href=\"" << "http://localhost:8080/" << "\">" << folders.substr(0, pin - 1) << "</a> / ";
		else
			ss << "<a href=\"" << "http://localhost:8080/" << "\">" << folders << "</a> / ";
		if (pin != std::string::npos)
			folders.erase(0, pin + 1);
		else
			folders.clear();
	}
	ss << "		</h1>" << std::endl;

	ss << "		<ul>" << std::endl;
	elem = readdir(d);
	while (elem != NULL)
	{
		if (elem->d_type == DT_DIR)
		{
			elem = readdir(d);
			continue;
		}
		ss << "			<li>" << std::endl; // -- CHANGE locahost TO NEW CONFIG
		ss << "				<a href=\"" << "http://localhost:8080" << src.path_uri << "/" << elem->d_name << "\"" << std::endl;
		ss << "				title=\"" << elem->d_name << "\">" << std::endl;
		ss << "				<span>" << elem->d_name << "</span>" << std::endl;
		ss << "				</a>" << std::endl;
		ss << "			</li>" << std::endl;
		elem = readdir(d);
	}
	closedir(d);
	ss << "		</ul>" << std::endl;

	ss << "	</div>" << std::endl;
	ss << "</body>" << std::endl;
	ss << "</html>" << std::endl;
	return (ss.str());
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
			// -- ISSUES: if a random file doenst have a file extention its just assumed as a location
			if (!(*(src.path_uri.end() - 1) == '/'))
				src.path_uri.append("/");
			if (src.path_uri != "/uploads") // remove when config added
				src.path_uri.append("index.html");
		}
		path = "www" + src.path_uri;
	}
	std::cout << RED "assembles path: " DEF << path << std::endl;
	return (path);
}

std::string Response::backup_error_page(t_status_code status_code)
{
	std::stringstream ss;
	ss << "<!DOCTYPE html>" << std::endl;
	ss << "<html lang=\"en\">" << std::endl;
	ss << "<head>" << std::endl;
	ss << "	<meta charset=\"UTF-8\">" << std::endl;
	ss << "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << std::endl;
	ss << "	<title>" << status_code << " " << HTTP::getReasonPhrase(status_code) << "</title>" << std::endl;
	ss << " <link rel=\"stylesheet\" href=\"/index.css\" type=\"text/css\">" << std::endl;
	ss << "</head>" << std::endl;
	ss << "<body>" << std::endl;
	ss << "<div class=\"text yellow-mark\"> Error: " << status_code << " " << HTTP::getReasonPhrase(status_code) << "</div>" << std::endl;
	ss << "</body>" << std::endl;
	ss << "</html>" << std::endl;
	return (ss.str());
}

void Response::method_post(Request &src)
{
	// create a special default location path to dump all POST info into
	// open the file and write to it

	// -- later get "www" replacement from config
	std::string path = "www" + src.path_uri + "database";
	std::ofstream output(path.c_str(), std::ofstream::out | std::ostream::app);

	if (!output.is_open())
		return ((status_code = INTERNAL_SERVER_ERROR), method_get(src));

	try
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

		output << body;
		output.close();
		headers["Location"] = src.path_uri;
	}
	catch (const std::exception &e)
	{
		// std::cerr << RED << e.what() << std::endl;
		status_code = INTERNAL_SERVER_ERROR;
		return (body.clear(), method_get(src));
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
	map_strings json_values;

	for (std::vector<MultiForm>::iterator it = src.multi_form.begin(); it != src.multi_form.end(); it++)
	{
		map_strings::iterator f_name = (*it).content_disposition.find("filename");
		if (f_name != (*it).content_disposition.end())
		{
			// -- adapt to config
			std::string path = "www" + src.path_uri + "uploads/" + random_name_generator();

			size_t len;
			len = (*f_name).second.rfind(".");
			if (len != std::string::npos)
				path += (*f_name).second.substr(len, (*f_name).second.length() - len - 1);

			// -- CAN I EVEN USE MKDIR????
			if (!opendir(("www" + src.path_uri + "uploads/").c_str()) && mkdir(("www" + src.path_uri + "uploads/").c_str(), 0777) == -1)
				throw(std::runtime_error("Internal Server Error"));
			std::fstream output(path.c_str(), std::ios::out);
			if (!output.is_open())
				throw(std::runtime_error("Internal Server Error"));

			output << (*it).data << CRLF;
			output.close();
		}
		else
		{
			// !! should this be in the database file?
			map_strings::iterator elem_name = (*it).content_disposition.find("name");
			std::string name = "name";
			if (elem_name != (*it).content_disposition.end())
				name = (*elem_name).second;
			json_values[name] = (*it).data;
		}
	}
	if (!json_values.empty())
	{
		body += "{";
		for (map_strings::iterator it = json_values.begin(); it != json_values.end();)
		{
			body += (*it).first + ":\"" + (*it).second + "\"";
			if (++it != json_values.end())
				body += ",";
		}
		body += "}" CRLF;
	}
	src.file_extension = "html";

	status_code = FOUND;
}

std::string Response::random_name_generator(void) const
{
	std::string name;
	int length = 12 + (rand() % 10);

	for (int i = 0; i < length; i++)
	{
		const int type = rand() % 3;
		switch (type)
		{
		case 0: // number
			name += '0' + rand() % 10;
			break;
		case 1: // lower-case
			name += 'a' + rand() % 26;
			break;
		default: // upper-case
			name += 'A' + rand() % 26;
			break;
		}
	}
	return (name);
}

void Response::method_delete(Request &src)
{
	// assemble the content path to delete
	std::string file_name = "www" + src.path_uri; // -- adapt to config

	struct stat sb;
	if (stat(file_name.c_str(), &sb) == -1) // resource doesn't exist
		return ((status_code = NOT_FOUND), method_get(src));

	if ((sb.st_mode & S_IFMT) == S_IFDIR) // don't allow folder DELETE
		return ((status_code = FORBIDDEN), method_get(src));

	// -- CAN I EVEN USE REMOVE??
	int res = std::remove(file_name.c_str());
	if (res == 0)
	{
		std::cout << "File deleted" << std::endl;
		status_code = NO_CONTENT; // success
		// can also be 200 OK if i want to send an html describing the outcome
	}
	else
	{
		std::cout << "No deletion" << std::endl;
		return ((status_code = INTERNAL_SERVER_ERROR), method_get(src));
	}
}

const char *Response::define_content_type(std::string &extension) const
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
	out << YEL "-- Response Information --" DEF "\n\n";
	out << YEL "PROTOCOL: " DEF << HTTP::stringProtocol(source.protocol) << std::endl;
	out << YEL "Status Code: " DEF << source.status_code << std::endl;
	out << YEL "Reason Phrase: " DEF << HTTP::getReasonPhrase(source.status_code) << std::endl;
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
