#include "../../inc/requests/Response.hpp"
#include "../../inc/requests/Request.hpp"

#include "../../inc/ansi_color_codes.h"

#include <fstream>	  // remove, fstream, ifstream, ofstream
#include <dirent.h>	  // opendir, readdir, closedir, DIR
#include <sys/stat.h> // mkdir, stat

namespace response_utils {

	std::string backup_error_page(t_status_code status);
	bool range_valid(size_t file_len, vector2 &ranges);
	std::string create_header_content_range(std::pair<int, int> range, t_status_code status, size_t size);
	const char *define_content_type(std::string &extension);
	std::string random_name_generator(void);
	std::string date_generate(void);

}

Response::Response(void) { clear(); }

Response::Response(Response const &src)
{
	*this = src;
}

Response::~Response(void) {}

Response &Response::operator=(Response const &src)
{
	if (this != &src)
	{
		this->protocol = src.protocol;
		this->boundary = src.boundary;
		this->status_code = src.status_code;
		this->headers = src.headers;
		this->body = src.body;
		this->cgi_reply = src.cgi_reply;
		this->full_response = src.full_response;
		set_state(src.get_state());
	}
	return (*this);
}

void Response::process(Request &src)
{
	src.set_state(BEGIN);
	

	/* while (full_response.empty())
	{
		// plan: make responses work in a state machine just like the requests
		std::cout << CYN "looping response state: " DEF << get_state() << std::endl;
		switch (get_state())
		{
		case BEGIN:
			clear();
			set_state(BODY);
			break;
		case BODY:
			//parse_body(request);
			break;
		case HEADERS:
			//set_response_headers(src);
			break;
		case END:
			//assemble_full_response(src);
			break;
		default:
			return;
		}
	} */

	if (!cgi_reply.empty())
	{
		std::string tmp = cgi_reply;
		clear();
		full_response = tmp;
		return;
	}
	clear();

	if (status_code != OK)
		src.method = GET;

	// check if the path is actually a redirection
	// HTTP redirect PERMANENT_REDIRECT
	// Location header to thet new url
	// no body
	try
	{
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
	}
	catch (const Response::CreateError &e)
	{
		std::cerr << e.what() << std::endl;
		status_code = e.response_status;
		body.clear();
		src.wanted_ranges.clear();
		method_get(src);
	}

	set_response_headers(src);
	assemble_full_response(src);
	
	std::cout << GRN "New Response ";
	std::cout << UYEL "has been processed" DEF << std::endl;
	std::cout << *this << std::endl;
}

void Response::clear(void)
{
	file_length = SIZE_NOT_SET;
	boundary.clear();
	
	protocol = H1_0;
	headers.clear();
	body.clear();
	cgi_reply.clear();
	full_response.clear();
	set_state(BEGIN);
}


void Response::set_state(t_http_state new_state) { state = new_state; }

t_http_state Response::get_state(void) const { return (state); }

size_t getMaxRequestBodySize(void)
{
	return (4096);
}

void Response::method_get(Request &src)
{
	// !! IF ERROR GO IN A DIFF ROUTE
	// get the requested content
	if (src.path_uri == "/uploads") // IN CASE OF AUTO INDEXING
	{
		body = create_autoindexing_page(src);
		return;
	}
	std::fstream file(assemble_content_path(src, status_code).c_str());
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
	{

		// seeing if response overflows body limit
		// get length of file:
		file.seekg(0, file.end);
		file_length = file.tellg();
		file.seekg(0, file.beg);
		std::cout << BLU "size of file for the body: " DEF << file_length << std::endl;

		//if (src.wanted_ranges.empty() && file_length > getMaxRequestBodySize())
			//src.wanted_ranges.push_back(std::pair<int, int>(0, getMaxRequestBodySize()));
			// idk what to do about this case yet

		// get content part by part
		if (!src.wanted_ranges.empty())
			body = create_range_response_body(file, file_length, src.file_extension, src.wanted_ranges);
		else
		{
			char buffer[40960]; // --- BAD
			file.read(buffer, static_cast<int>(file_length));
			body = to_str(buffer);
			//body = to_str(file.rdbuf());
		}
		file.close();
	}
	else
		body = response_utils::backup_error_page(status_code);
}

std::string Response::create_range_response_body(std::fstream &file, size_t file_len, std::string ext, vector2 &ranges)
{
	if (!response_utils::range_valid(file_len, ranges))
		throw(Response::CreateError("Incorrect Range header value", REQUESTED_RANGE_NOT_SATISFIABLE));

	if (ranges.size() > 1)
		return (multiple_range(file, file_len, ext, ranges));
	else
		return (single_range(file, *(ranges.begin())));
}

std::string Response::multiple_range(std::fstream &file, size_t file_len, std::string ext, vector2 &ranges)
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
		content += "Content-Type: " + to_str(response_utils::define_content_type(ext));
		content += "Content-Range: " + response_utils::create_header_content_range(ranges[i], OK, file_len);
		content += single_range(file, ranges[i]);
		content += CRLF;
	}
	content += "--" + boundary + "--";
	return(content);
}

std::string Response::single_range(std::fstream &file, std::pair<int, int> range)
{
	// !! SINGLE RANGE
	// (make this one a function that returns std::string so i can use it inside MULTI RANGE)
	// -- step 1: read and add selected range part of file
	// ... done
	
	char buffer[40960]; // ------------------------- NOT A GOOD SOLUTION...

	// char buffer[4096];
	// file.read(buffer, getMaxRequestBodySize());
	// -- read: rdbuf vs. read
	// rdbuf returns a pointer to the original buffer for the string
	// so it cannot be used to send just a part of the content
	std::cout << "first: " << range.first << std::endl;
	std::cout << "last: " << range.second - range.first << std::endl;
	//file.seekg(range.first);

	file.read(buffer, static_cast<int>(getMaxRequestBodySize()) - 1);
	/* if (range.second - range.first > static_cast<int>(getMaxRequestBodySize()))
		file.read(buffer, static_cast<int>(getMaxRequestBodySize()));
	else
		file.read(buffer, range.second - range.first); */
	/* if (file)
      std::cout << "all characters read successfully.";
    else */
    std::cout << "characters read: only " << file.gcount() << " could be read";
	std::cout << RED "BUFFER SINGLE RANGE:\n" DEF;
	std::cout << buffer;
	std::cout << RED "\n--end of body buffer\n" DEF;
	std::string content(buffer); //, range.second - range.first); // SEGMENTATION FAULT NOT WORKING DONR COMMIT PLEASE
	std::cout << RED "BODY CONTENT SINGLE RANGE:\n" DEF;
	std::cout << content;
	std::cout << RED "\n--end of body content\n" DEF;

	return (content);
}

std::string Response::create_autoindexing_page(Request &src)
{
	DIR *d = opendir(("www" + src.path_uri).c_str());
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

void Response::method_post(Request &src)
{
	// create a special default location path to store all POST info into
	// open the file/folder and write to it

	// -- later get "www" replacement from config
	std::string path = "www" + src.path_uri + "database";
	std::ofstream output(path.c_str(), std::ofstream::out | std::ostream::app);

	if (!output.is_open())
		throw(Response::CreateError("Was unable to open database file", INTERNAL_SERVER_ERROR));

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
			std::string path = "www" + src.path_uri + "uploads/" + response_utils::random_name_generator();

			size_t len;
			len = (*f_name).second.rfind(".");
			if (len != std::string::npos)
				path += (*f_name).second.substr(len, (*f_name).second.length() - len - 1);

			// -- CAN I EVEN USE MKDIR????
			if (!opendir(("www" + src.path_uri + "uploads/").c_str()) \
					&& mkdir(("www" + src.path_uri + "uploads/").c_str(), 0777) == -1)
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
	src.file_extension = "html";
	status_code = FOUND;
}

void Response::method_delete(Request &src)
{
	// assemble the content path to delete
	std::string file_name = "www" + src.path_uri; // -- adapt to config

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
		status_code = NO_CONTENT; // success
		// can also be 200 OK if i want to send an html describing the outcome
	}
	else
		throw(Response::CreateError("Was unable to delete file", INTERNAL_SERVER_ERROR));
}

void Response::set_response_headers(Request &src)
{
	if (src.wanted_ranges.size() == 1 || status_code == REQUESTED_RANGE_NOT_SATISFIABLE)
		headers["Content-Range"] = response_utils::create_header_content_range(*(src.wanted_ranges.begin()), status_code, file_length);

	headers["Content-Length"] = to_str(body.size());
	
	if (src.wanted_ranges.size() < 2) 
		headers["Content-Type"] = response_utils::define_content_type(src.file_extension);
	else
		headers["Content-Type"] = "multipart/byteranges; boundary=" + boundary;
	
	if (src.headers.find("connection") != src.headers.end())
		headers["Connection"] = src.headers["connection"];
	else
		headers["Connection"] = "keep-alive";

	// use stat() to get last modified header
	headers["Date"] = response_utils::date_generate();
}

void Response::assemble_full_response(Request &src)
{
	std::stringstream ss;
	
	ss << HTTP::stringProtocol(src.protocol) << " " << status_code << " " << HTTP::getReasonPhrase(status_code);
	ss << CRLF;
	for (map_strings::iterator it = headers.begin(); it != headers.end(); it++)
		ss << (*it).first << ": " << (*it).second << CRLF;
	ss << CRLF;
	ss << body;
	full_response = ss.str();
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
	if (src.headers["Content-Type"] == "image/vnd.microsoft.icon")
		out << "[PAGE ICON IMAGE]" << std::endl;
	else
		out << src.body << std::endl;
	/*
	if (!src.full_response.empty())
		out << YEL "Full Response..." DEF << std::endl << src.full_response << std::endl;
	*/
	out << std::endl;
	return (out);
}
