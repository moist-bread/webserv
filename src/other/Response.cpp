#include "../../inc/requests/Response.hpp"
#include "../../inc/requests/Request.hpp"

#include "../../inc/ansi_color_codes.h"

#include <fstream>	  // remove, fstream, ifstream, ofstream
#include <dirent.h>	  // opendir, readdir, closedir, DIR
#include <sys/stat.h> // mkdir, stat
#include <ctime>	  // time

#define BUFFER_SIZE 4096

namespace response_utils
{

	std::string backup_error_page(t_status_code status);
	bool range_valid(int file_len, vector2 &ranges);
	std::string create_header_content_range(std::pair<int, int> range, t_status_code status, int size);
	const char *define_content_type(std::string &extension);
	std::string random_name_generator(void);
	std::string date_format(time_t timestamp);

}

Response::Response(void) : req(NULL) { clear(); }

Response::Response(Request &src) : req(&src) { clear(); }

Response::Response(Response const &src)
{
	*this = src;
}

Response::~Response(void) {}

Response &Response::operator=(Response const &src)
{
	if (this != &src)
	{
		this->file_length = src.file_length;
		this->boundary = src.boundary;

		this->protocol = src.protocol;
		this->status_code = src.status_code;
		this->headers = src.headers;
		this->body = src.body;

		this->cgi_reply = src.cgi_reply;

		this->full_response = src.full_response;

		set_state(src.get_state());
	}
	return (*this);
}

void Response::process(void)
{
	while (full_response.empty())
	{
		std::cout << CYN "looping response state: " DEF << get_state() << std::endl;
		switch (get_state())
		{
		case PREP:
			preparations_for_response();
			break;
		case CGI:
			// send the cgi to be executed here
			break;
		case METHODS:
			execute_methods();
			break;
		case HEADERS_RESP:
			set_response_headers();
			break;
		case FULL_RESP:
			assemble_full_response();
			break;
		default:
			set_state(SEND);
			return;
		}
		if (get_state() == SEND)
			break;
	}

	std::cout << GRN "New Response ";
	std::cout << UYEL "has been processed" DEF << std::endl;
	std::cout << *this << std::endl;
}

void Response::clear(void)
{
	file_length = VALUE_NOT_SET;
	boundary.clear();

	protocol = H1_0;
	headers.clear();
	body.clear();
	cgi_reply.clear();
	full_response.clear();
	set_state(PREP);
}

void Response::preparations_for_response(void)
{
	if (!this->req)
		throw(std::runtime_error("Response was initialized without associated Request"));
	(*req).set_state(BEGIN);

	// !! check if the path is actually a redirection
	// HTTP redirect PERMANENT_REDIRECT
	// Location header to thet new url
	// no body

	// -- CHECK FOR CGI EXECUTION
	// set_state(CGI);

	// -- DO SOMETHING ELSE FOR CGI RESPONSE ASSEMBLY
	if (!cgi_reply.empty())
	{
		std::cout << YEL "creating cgi full response..." DEF << std::endl;
		std::string tmp = cgi_reply;
		clear();
		full_response = tmp;
		set_state(SEND);
		return;
	}
	clear();

	if (status_code != OK)
		(*req).method = GET;
	set_state(METHODS);
}

void Response::execute_methods(void)
{
	try
	{
		switch ((*req).method)
		{
		case GET:
			method_get();
			break;
		case POST:
			method_post();
			break;
		case DELETE:
			method_delete();
			break;
		default:
			status_code = METHOD_NOT_ALLOWED;
			method_get();
			break;
		}
	}
	catch (const Response::CreateError &e)
	{
		std::cerr << e.what() << std::endl;
		status_code = e.response_status;
		body.clear();
		(*req).wanted_ranges.clear();
		// --------- change this please
		method_get(); // replace with error wtv
	}
	set_state(HEADERS_RESP);
}

void Response::set_state(t_response_state new_state) { state = new_state; }

t_response_state Response::get_state(void) const { return (state); }

void Response::method_get(void)
{
	// !! IF ERROR GO IN A DIFF ROUTE
	// get the requested content
	if ((*req).path_uri == "/uploads") // IN CASE OF AUTO INDEXING
	{
		body = create_autoindexing_page();
		return;
	}
	std::ifstream file(assemble_content_path(status_code).c_str());
	if (!file.is_open())
	{
		perror("File does not exist");
		if ((*req).file_extension == "html")
			status_code = NOT_FOUND;
		else
			status_code = INTERNAL_SERVER_ERROR;
		(*req).file_extension = "html";
		file.open(assemble_content_path(status_code).c_str());
	}

	if (file.is_open())
	{
		file.seekg(0, file.end);
		int len = file.tellg();
		file.seekg(0, file.beg);

		if (!(*req).wanted_ranges.empty())
		{
			file_length = len;
			body = create_range_response_body(file, (*req).wanted_ranges);
		}
		else
			body = to_str(file.rdbuf());
		file.close();
	}
	else
		body = response_utils::backup_error_page(status_code);
}

std::string Response::create_range_response_body(std::ifstream &file, vector2 &ranges)
{
	if (!response_utils::range_valid(file_length, ranges))
		throw(Response::CreateError("Incorrect Range header value", RANGE_NOT_SATISFIABLE));

	status_code = PARTIAL_CONTENT;
	if (ranges.size() > 1)
		return (multiple_range(file, ranges));
	else
		return (single_range(file, *(ranges.begin())));
}

std::string Response::multiple_range(std::ifstream &file, vector2 &ranges)
{
	// !! MULTI RANGE
	// -- step 1: create boundary string
	// (translation: 1-70 digits, last can't be a space)

	// boundary := 0*69<bchars> bcharsnospace
	// bchars := bcharsnospace / " "
	// bcharsnospace :=    DIGIT / ALPHA / "'" / "(" / ")" / "+"  / "_"
	//			/ "," / "-" / "." / "/" / ":" / "=" / "?"
	boundary = response_utils::random_name_generator();

	// ... in a loop
	// -- step 2: add "--boundary" and headers
	// -- step 3: read and add selected range part of file until end of ranges
	// ... end of loop
	// -- step 4: finish with "--boundary--"

	std::string content;

	for (size_t i = 0; i < ranges.size(); i++)
	{
		content += "--" + boundary + CRLF;
		content += "Content-Type: " + to_str(response_utils::define_content_type((*req).file_extension));
		content += CRLF;
		content += "Content-Range: " + response_utils::create_header_content_range(ranges[i], OK, file_length);
		content += CRLF;
		content += CRLF;
		content += single_range(file, ranges[i]);
		content += CRLF;
	}
	content += "--" + boundary + "--";
	return (content);
}

std::string Response::single_range(std::ifstream &file, std::pair<int, int> range)
{
	// !! SINGLE RANGE

	char buffer[BUFFER_SIZE];
	std::string content;
	int read_progress = range.first;
	int read_end = range.second;
	// ----------- POSSIBLE CHNGE: maybe limit read_end a bit when the file is way too big
	//		to avoid the change of having a 5GB std::string content
	//		happens when: normal get on big file, any big get range request

	while (read_progress < read_end)
	{
		file.seekg(read_progress);
		int to_read = (read_end - read_progress > BUFFER_SIZE ? BUFFER_SIZE : read_end - read_progress);
		file.read(buffer, to_read);
		if (!file)
			throw(Response::CreateError("Was unable to read from file", INTERNAL_SERVER_ERROR));
		// std::cout << "characters read: " << file.gcount() << std::endl;
		// std::cout << YEL "BUFFER READ FROM " << read_progress << " UNTIL ";
		read_progress += file.gcount();
		std::string tmp(buffer, file.gcount());
		content += tmp;
		// std::cout << read_progress << ":\n" DEF;
		// std::cout << tmp;
		// std::cout << YEL "\n--end of buffer\n" DEF;
	}
	return (content);
}

std::string Response::create_autoindexing_page(void)
{
	DIR *d = opendir(("www" + (*req).path_uri).c_str());
	struct dirent *elem;
	if (!d)
	{
		// ------------- maybe throw instead
		status_code = NOT_FOUND;
		return (response_utils::backup_error_page(status_code));
	}

	std::stringstream ss;
	ss << "<!DOCTYPE html>" << std::endl;
	ss << "<html lang=\"en\">" << std::endl;
	ss << "<head>" << std::endl;
	ss << "	<meta charset=\"UTF-8\">" << std::endl;
	ss << "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << std::endl;
	ss << "	<title>listing directory www/" << (*req).path_uri << "</title>" << std::endl; // -- CHANGE WWW TO NEW CONFIG
	ss << " 	<link rel=\"stylesheet\" href=\"/index.css\" type=\"text/css\">" << std::endl;
	ss << "	</head>" << std::endl;
	ss << "	<body>" << std::endl;

	ss << "	<div id=\"wrapper\">" << std::endl;

	ss << "		<h1>";
	ss << "<a href=\"" << "http://localhost:8080/" << "\">~</a> / "; // -- CHANGE locahost TO NEW CONFIG
	std::string folders = (*req).path_uri;
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
		ss << "				<a href=\"" << "http://localhost:8080" << (*req).path_uri << "/" << elem->d_name << "\"" << std::endl;
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

std::string Response::assemble_content_path(t_status_code status_code)
{
	std::string path;
	if (status_code != OK)
	{
		// in case of error
		// look at the config to see which error page to send
		// (later) consider default error pages from config
		path = "www/404.html";
		(*req).file_extension = "html";
	}
	else
	{
		// in case of everything being fine
		// do path with
		// (later) consider locations from config
		if ((*req).file_extension == "html")
		{
			// -- ISSUES: if a random file doenst have a file extention its just assumed as a location
			if (!(*((*req).path_uri.end() - 1) == '/'))
				(*req).path_uri.append("/");
			if ((*req).path_uri != "/uploads") // remove when config added
				(*req).path_uri.append("index.html");
		}
		path = "www" + (*req).path_uri;
	}
	(*req).path_uri = path;
	std::cout << RED "assembles path: " DEF << path << std::endl;
	return (path);
}

void Response::method_post(void)
{
	// create a special default location path to store all POST info into
	// open the file/folder and write to it

	// -- later get "www" replacement from config
	std::string path = "www" + (*req).path_uri + "database";
	std::ofstream output(path.c_str(), std::ofstream::out | std::ostream::app);

	if (!output.is_open())
		throw(Response::CreateError("Was unable to open database file", INTERNAL_SERVER_ERROR));

	if (!(*req).json.empty())
		handle_application_form();
	else if (!(*req).multi_form.empty())
		handle_multipart_form();
	else
	{
		body = (*req).body;
		(*req).file_extension = "html";
	}
	output << body;
	output.close();
	headers["Location"] = (*req).path_uri;
}

void Response::handle_application_form(void)
{
	// json to the body...
	body = (*req).json + CRLF;
	(*req).file_extension = "json";

	// FOUND will make the client request a GET for a redirection location
	status_code = FOUND;
}

void Response::handle_multipart_form(void)
{
	map_strings json_values;

	for (std::vector<MultiForm>::iterator it = (*req).multi_form.begin(); it != (*req).multi_form.end(); it++)
	{
		map_strings::iterator f_name = (*it).content_disposition.find("filename");
		if (f_name != (*it).content_disposition.end())
		{
			// -- adapt to config
			std::string path = "www" + (*req).path_uri + "uploads/" + response_utils::random_name_generator();

			size_t len;
			len = (*f_name).second.rfind(".");
			if (len != std::string::npos)
				path += (*f_name).second.substr(len, (*f_name).second.length() - len - 1);

			// -- CAN I EVEN USE MKDIR????
			if (!opendir(("www" + (*req).path_uri + "uploads/").c_str()) && mkdir(("www" + (*req).path_uri + "uploads/").c_str(), 0777) == -1)
				throw(Response::CreateError("Was unable to open uploads directory", INTERNAL_SERVER_ERROR));
			std::fstream output(path.c_str(), std::ios::out);
			if (!output.is_open())
				throw(Response::CreateError("Was unable to upload file", INTERNAL_SERVER_ERROR));

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
	(*req).file_extension = "html";
	status_code = FOUND;
}

void Response::method_delete(void)
{
	// assemble the content path to delete
	std::string file_name = "www" + (*req).path_uri; // -- adapt to config

	struct stat sb;
	if (stat(file_name.c_str(), &sb) == -1) // resource doesn't exist
		throw(Response::CreateError("Could not find desired file", NOT_FOUND));

	if ((sb.st_mode & S_IFMT) == S_IFDIR) // don't allow folder DELETE
		throw(Response::CreateError("Action not permited", FORBIDDEN));

	// -- CAN I EVEN USE REMOVE??
	int res = std::remove(file_name.c_str());
	if (res == 0)
	{
		std::cout << "File deleted" << std::endl;
		// can also be 200 OK if i want to send an html describing the outcome
		status_code = NO_CONTENT; // success
	}
	else
		throw(Response::CreateError("Was unable to delete file", INTERNAL_SERVER_ERROR));
}

void Response::set_response_headers(void)
{
	if ((*req).wanted_ranges.size() == 1 || status_code == RANGE_NOT_SATISFIABLE)
		headers["Content-Range"] = response_utils::create_header_content_range(*((*req).wanted_ranges.begin()), status_code, file_length);

	headers["Content-Length"] = to_str(body.size());

	if (!body.empty())
	{
		struct stat sb;
		if (stat((*req).path_uri.c_str(), &sb) != -1)
			headers["Last-Modified"] = response_utils::date_format(sb.st_mtime);

		if ((*req).wanted_ranges.size() < 2)
			headers["Content-Type"] = response_utils::define_content_type((*req).file_extension);
		else
			headers["Content-Type"] = "multipart/byteranges; boundary=" + boundary;
	}

	if ((*req).headers.find("connection") != (*req).headers.end())
		headers["Connection"] = (*req).headers["connection"];
	else
		headers["Connection"] = "keep-alive";

	headers["Date"] = response_utils::date_format(std::time(NULL));
	set_state(FULL_RESP);
}

void Response::assemble_full_response(void)
{
	std::stringstream ss;

	ss << HTTP::stringProtocol(H1_0) << " " << status_code << " " << HTTP::getReasonPhrase(status_code);
	ss << CRLF;
	for (map_strings::iterator it = headers.begin(); it != headers.end(); it++)
		ss << (*it).first << ": " << (*it).second << CRLF;
	ss << CRLF;
	ss << body;
	full_response = ss.str();
	set_state(SEND);
}

std::ostream &operator<<(std::ostream &out, Response &src)
{
	out << YEL "-- Response Information --" DEF "\n\n";
	out << YEL "PROTOCOL: " DEF << HTTP::stringProtocol(src.protocol) << std::endl;
	out << YEL "Status Code: " DEF << src.status_code << std::endl;
	out << YEL "Reason Phrase: " DEF << HTTP::getReasonPhrase(src.status_code) << std::endl;
	out << YEL "Headers..." DEF << std::endl;
	for (map_strings::iterator it = src.headers.begin(); it != src.headers.end(); it++)
		out << YEL "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;
	out << YEL "Body..." DEF << std::endl;
	if (src.headers["Content-Type"] == "image/vnd.microsoft.icon" || src.headers["Content-Type"] == "image/png" || src.headers["Content-Type"] == "video/mp4" )
		out << "[IMAGE]" << std::endl;
	else
		out << src.body << std::endl;
/* 
	if (!src.full_response.empty())
		out << YEL "Full Response..." DEF << std::endl << src.full_response << std::endl;
	out << std::endl;
*/
	return (out);
}
