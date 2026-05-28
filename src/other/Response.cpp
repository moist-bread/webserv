#include "../../inc/requests/Response.hpp"
#include "../../inc/requests/Request.hpp"
#include "../../inc/requests/Inspect.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include "../../inc/serverConfig/LocationConfig.hpp"

#include "../../inc/string_utils.tpp"
#include "../../inc/ansi_color_codes.h"

#include <fstream>		// remove, fstream, ifstream, ofstream
#include <dirent.h>		// opendir, readdir, closedir, DIR
#include <sys/stat.h>	// mkdir, stat
#include <unistd.h>		// access
#include <ctime>		// time

#define BUFFER_SIZE 4096

namespace response_utils
{

	bool is_error(t_status_code code);
	std::string backup_error_page(t_status_code status);
	std::string directory_listing(DIR *d, const std::string listen_str, const std::string folder_str);
	bool range_valid(int file_len, vector2 &ranges);
	std::string create_header_content_range(std::pair<int, int> range, t_status_code status, int size);
	const char *define_content_type(std::string &extension);
	std::string random_name_generator(void);
	std::string date_format(time_t timestamp);

}

Response::Response(void) : req(NULL), conf(NULL) { clear(true); }

Response::Response(Request &req_ref, const ServerConfig *sc) : req(&req_ref), conf(sc) { clear(true); }

Response::Response(const Response &src) { *this = src; }

Response::~Response(void) {}

Response &Response::operator=(const Response &src)
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
		this->is_chunked = src.is_chunked;

		this->full_response = src.full_response;

		set_state(src.get_state());
	}
	return (*this);
}

void Response::process(void)
{
	while (full_response.empty())
	{
		// std::cout << CYN "looping response state: " DEF << get_state() << std::endl;
		switch (get_state())
		{
		case PREP:
			preparations_for_response();
			break;
		case METHODS:
			execute_methods();
			break;
		case CGI:
			cgi_response();
			break;
		case CHUNK:
			chunk_response();
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

	// std::cout << *this << std::endl;
}

void Response::set_state(const t_response_state &new_state) { state = new_state; }

const t_response_state &Response::get_state(void) const { return (state); }

void Response::clear(bool all)
{
	file_length = VALUE_NOT_SET;
	boundary.clear();

	protocol = H1_1;
	headers.clear();
	body.clear();
	if (all)
		cgi_reply.clear();
	is_chunked = false;
	full_response.clear();
	set_state(PREP);
}

void Response::preparations_for_response(void)
{
	if (!this->req)
		throw(std::runtime_error("Response was initialized without associated Request"));
	else if (!this->conf)
		throw(std::runtime_error("Response was initialized without associated ServerConfig"));

	(*req).set_state(BEGIN);

	if (!is_chunked && !cgi_reply.empty() && !response_utils::is_error(status_code))
		return (set_state(CGI));
	else if (is_chunked && !response_utils::is_error(status_code))
		return (set_state(CHUNK));

	clear(true);
	if ((*req).protocol != UNSUPPORTED_PROTOCOL)
		protocol = (*req).protocol;

	if (!(*req).loc)
	{
		if (!response_utils::is_error(status_code))
			status_code = NOT_FOUND;
	}
	else if ((*req).loc->isRedirect())
	{
		// !! this isn't working properly yet
		headers["Location"] = (*req).loc->getReturnUrl();
		status_code = (*req).loc->getReturnCode();
		return (set_state(HEADERS_RESP));
	}
	
	set_state(METHODS);
}

void Response::execute_methods(void)
{
	if (response_utils::is_error(status_code) && (*req).method == HEAD)
		return(set_state(HEADERS_RESP));


	if (response_utils::is_error(status_code))
		(*req).method = GET;
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
		case HEAD:
			method_head();
			break;
		default:
			throw(Response::CreateError("Unsupported Method", METHOD_NOT_ALLOWED));
			break;
		}
	}
	catch (const Response::CreateError &e)
	{
		Inspect::inspect_response_activity(e.what(), *this);
		status_code = e.response_status;
		(*req).wanted_ranges.clear();
		error_response();
	}
	set_state(HEADERS_RESP);
}

void Response::cgi_response(void)
{
	// std::cout << YEL "starting cgi response..." DEF << std::endl;
	clear(false);

	// -- step 1: extract headers (and or line) from cgi_reply
	std::string preamble;
	size_t pin;

	pin = cgi_reply.find(CRLF CRLF);
	if (pin == std::string::npos)
		return ((status_code = INTERNAL_SERVER_ERROR), set_state(METHODS)); // ------------ could be a problem
	preamble = cgi_reply.substr(0, pin);
	cgi_reply.erase(0, pin + 2);

	if (preamble.compare(0, 5, "HTTP/") == 0)
	{
		pin = preamble.find("\n");
		if (pin == std::string::npos)
			return ((status_code = INTERNAL_SERVER_ERROR), set_state(METHODS)); // ------------ could be a problem
		preamble.erase(0, pin + 1);
	}

	// -- step 2: add those headers
	headers = HTTP::extract_key_value(&preamble, ":", CRLF);

	// -- step 3: update status_code based on Status header
	if (headers.find("status") != headers.end())
	{
		try
		{
			int st = stringToNumber<int>(headers["status"], std::dec);
			if (HTTP::isValidReasonPhrase(st) == INVALID_CODE)
				throw(std::runtime_error("Invalid cgi Status header"));
			status_code = static_cast<t_status_code>(st);
		}
		catch(...) { ; }
	}
	set_state(CHUNK);
}

void Response::chunk_response(void)
{
	// std::cout << YEL "processing chunk..." DEF << std::endl;

	std::stringstream ss;
	ss << std::hex << cgi_reply.size() << std::dec << CRLF;
	ss << cgi_reply << CRLF;
	body = ss.str();

	if (!is_chunked)
	{
		is_chunked = true;
		set_state(HEADERS_RESP);
	}
	else
	{
		if (cgi_reply.empty())
			is_chunked = false;
		full_response = body;
		set_state(SEND);
	}
}

void Response::set_response_headers(void)
{
	if ((*req).wanted_ranges.size() == 1)
		headers["Content-Range"] = response_utils::create_header_content_range((*req).wanted_ranges[0], status_code, file_length);
	else if (status_code == RANGE_NOT_SATISFIABLE)
		headers["Content-Range"] = response_utils::create_header_content_range(std::pair<int, int>(VALUE_NOT_SET, VALUE_NOT_SET), status_code, file_length);

	if (is_chunked)
	{
		headers["Transfer-Encoding"] = "chunked";
		headers.erase("content-length");
		protocol = H1_1;
	}
	else if (headers.find("Content-Length") == headers.end())
		headers["Content-Length"] = to_str(body.size());

	if (!body.empty())
	{
		struct stat sb;
		if (stat((*req).path_uri.c_str(), &sb) != -1)
			headers["Last-Modified"] = response_utils::date_format(sb.st_mtime);

		if ((*req).wanted_ranges.size() < 2 && !is_chunked)
			headers["Content-Type"] = response_utils::define_content_type((*req).file_extension);
		else if ((*req).wanted_ranges.size() > 1)
			headers["Content-Type"] = "multipart/byteranges; boundary=" + boundary;
	}

	// close, else it will continue trying to send content
	if (status_code == CONTENT_TOO_LARGE)
		headers["Connection"] = "close";
	else if ((*req).headers.find("connection") != (*req).headers.end())
		headers["Connection"] = (*req).headers["connection"];
	else
		headers["Connection"] = "keep-alive";

	headers["Date"] = response_utils::date_format(std::time(NULL));
	set_state(FULL_RESP);
}

void Response::assemble_full_response(void)
{
	std::stringstream ss;

	ss << HTTP::stringProtocol(protocol) << " " << status_code << " " << HTTP::getReasonPhrase(status_code);
	ss << CRLF;
	for (map_strings::const_iterator it = headers.begin(); it != headers.end(); it++)
		ss << (*it).first << ": " << (*it).second << CRLF;
	ss << CRLF;
	ss << body;
	full_response = ss.str();
	set_state(SEND);
}

void Response::method_get(void)
{
	if (response_utils::is_error(status_code))
		return (error_response());

	if ((*req).loc && (*req).loc->isAutoIndex((*req).path_uri))
	{
		body = autoindexing_page();
		return;
	}
	std::ifstream file(assemble_content_path().c_str());
	if (!file.is_open())
	{
		// ---------------- see if 404 is really the only option
		throw(Response::CreateError("File did not open", NOT_FOUND));
		// if ((*req).file_extension == "html")
		//else
		//	throw(Response::CreateError("File did not open", INTERNAL_SERVER_ERROR));
	}
	else
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
}

void Response::error_response(void)
{
	(*req).file_extension = "html";
	std::ifstream file(assemble_content_path().c_str());
	if (!file.is_open())
	{
		// std::cout << "Defined error file does not exist, using backup" << std::endl;
		body = response_utils::backup_error_page(status_code);
	}
	else
	{
		body = to_str(file.rdbuf());
		file.close();
	}
}

std::string Response::autoindexing_page(void) const
{
	DIR *d = opendir(((*req).loc->getRoot() + (*req).path_uri.substr((*req).loc->getPath().length())).c_str());
	if (!d)
		throw(Response::CreateError("Could not find desired folder", NOT_FOUND));

	std::string indexing_page = response_utils::directory_listing(d, conf->getServerUrl(), (*req).path_uri);
	closedir(d);
	return (indexing_page);
}

std::string Response::assemble_content_path(void)
{
	std::string path;
	if (response_utils::is_error(status_code))
	{
		(*req).file_extension = "html";
		path = conf->getErrorPage(status_code);
		if (path.empty())
			return (path);
		path.insert(0, conf->getRoot());
	}
	else
	{
		path = (*req).loc->getRoot();

		if ((*req).file_extension == "html")
		{
			// -- see which of the indexes is valid
			std::string index;
			for (size_t i = 0; i < (*req).loc->getIndexes().size() && index.empty(); i++)
			{
				// std::cout << "which index is valid: " <<  (path + "/" + (*req).loc->getIndexes()[i]) << std::endl;
				if (access((path + "/" + (*req).loc->getIndexes()[i]).c_str(), R_OK) != 0)
					continue;
				struct stat path_stat;
				if (stat((path + "/" + (*req).loc->getIndexes()[i]).c_str(), &path_stat) != 0)
					continue;
				if (S_ISDIR(path_stat.st_mode))
					continue;
				// std::cout << "found ..." << std::endl;
				index = (*req).loc->getIndexes()[i];
			}

			// -- if one of them was valid, add to the path
			if (!index.empty())
			{
				if ((*req).path_uri[(*req).path_uri.length() - 1] != '/')
					(*req).path_uri.append("/");
				(*req).path_uri.append(index);
			}
		}
		
		path += (*req).path_uri.substr((*req).loc->getPath().length());
	}
	(*req).path_uri = path;
	if (Inspect::debug)
		std::cout << RED "assembled path: " DEF << path << std::endl;
	return (path);
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

std::string Response::multiple_range(std::ifstream &file, const vector2 &ranges)
{
	// !! MULTI RANGE

	// boundary := 0*69<bchars> bcharsnospace
	// bchars := bcharsnospace / " "
	// bcharsnospace :=    DIGIT / ALPHA / "'" / "(" / ")" / "+"  / "_"
	//			/ "," / "-" / "." / "/" / ":" / "=" / "?"

	// (translation: 1-70 digits, last can't be a space)
	boundary = response_utils::random_name_generator();

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

std::string Response::single_range(std::ifstream &file, const std::pair<int, int> &range) const
{
	// !! SINGLE RANGE

	char buffer[BUFFER_SIZE];
	std::string content;
	int read_progress = range.first;
	int read_end = range.second;

	while (read_progress < read_end)
	{
		file.seekg(read_progress);
		int to_read = (read_end - read_progress > BUFFER_SIZE ? BUFFER_SIZE : read_end - read_progress);
		file.read(buffer, to_read);
		if (!file)
			throw(Response::CreateError("Was unable to read from file", INTERNAL_SERVER_ERROR));
		read_progress += file.gcount();
		std::string tmp(buffer, file.gcount());
		content += tmp;
	}
	return (content);
}

void Response::method_post(void)
{
	std::string path = (*req).loc->getUploadStore() + "database";
	
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
	body = (*req).json + CRLF;
	(*req).file_extension = "json";

	// will make the client request to a redirection location
	status_code = FOUND;
}

void Response::handle_multipart_form(void)
{
	map_strings json_values;

	for (std::vector<MultiForm>::iterator it = (*req).multi_form.begin(); it != (*req).multi_form.end(); it++)
	{
		map_strings::iterator elem_name = (*it).content_disposition.find("name");
		std::string name = "name";
		if (elem_name != (*it).content_disposition.end())
			name = (*elem_name).second;
		
		map_strings::iterator f_name = (*it).content_disposition.find("filename");
		if (f_name != (*it).content_disposition.end())
		{
			// -- see if the uploads folder exists
			std::string path = (*req).loc->getUploadStore();
			
			if (!opendir(path.c_str())) // && mkdir(path.c_str(), 0777) == -1) // -- use of mkdir is not allowed
				throw(Response::CreateError("Was unable to open uploads directory", INTERNAL_SERVER_ERROR));
			
			// -- create a file with a randomly generated name
			std::string rand_name = response_utils::random_name_generator();
			size_t len;
			len = (*f_name).second.rfind(".");
			if (len != std::string::npos)
				rand_name += (*f_name).second.substr(len, (*f_name).second.length() - len - 1);
			path += rand_name;

			std::fstream output(path.c_str(), std::ios::out);
			if (!output.is_open())
				throw(Response::CreateError("Was unable to upload file", INTERNAL_SERVER_ERROR));

			// -- write to the file, and save the info to later put in the database
			output << (*it).data << CRLF;
			output.close();
			
			// -- save the info to later put in the database
			json_values[name] = rand_name;
		}
		else
		{
			// -- save the info to later put in the database
			json_values[name] = (*it).data;
		}
	}

	if (!json_values.empty())
	{
		// -- converting the info into json and into the body to later put in the database
		body += "{";
		for (map_strings::const_iterator it = json_values.begin(); it != json_values.end();)
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
	std::string file_name = (*req).loc->getRoot() + (*req).path_uri.substr((*req).loc->getPath().length());

	struct stat sb;
	if (stat(file_name.c_str(), &sb) == -1)
		throw(Response::CreateError("Could not find desired file", NOT_FOUND));

	if (S_ISDIR(sb.st_mode)) // don't allow folder DELETE
		throw(Response::CreateError("Action not permited", FORBIDDEN));

	int res = std::remove(file_name.c_str());
	if (res == 0)
	{
		// can also be 200 OK if i want to send an html describing the outcome
		status_code = NO_CONTENT; // success
	}
	else
		throw(Response::CreateError("Was unable to delete file", INTERNAL_SERVER_ERROR));
}

void Response::method_head(void)
{
	// HEAD is like a scouting version of GET
	// it only wants to see the size of the content and ignores the response body
	try
	{
		method_get();
	}
	catch (const Response::CreateError &e)
	{
		Inspect::inspect_response_activity(e.what(), *this);
		status_code = e.response_status;
		(*req).wanted_ranges.clear();
		body.clear();
	}
	
	headers["Content-Length"] = to_str(body.size());
	body.clear();
}

std::ostream &operator<<(std::ostream &out, const Response &src)
{
	out << YEL "-- Response Information --" DEF "\n\n";
	out << YEL "PROTOCOL: " DEF << HTTP::stringProtocol(src.protocol) << std::endl;
	out << YEL "Status Code: " DEF << src.status_code << std::endl;
	out << YEL "Reason Phrase: " DEF << HTTP::getReasonPhrase(src.status_code) << std::endl;
	out << YEL "Headers..." DEF << std::endl;
	for (map_strings::const_iterator it = src.headers.begin(); it != src.headers.end(); it++)
		out << YEL "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;
	out << YEL "Body..." DEF;
	out << " (size) " << src.body.size() << std::endl;
	/*
	if (src.headers["Content-Type"] == "image/vnd.microsoft.icon" || src.headers["Content-Type"] == "image/png" || src.headers["Content-Type"] == "video/mp4" )
		out << "[IMAGE]" << std::endl;
	else
		out << src.body << std::endl;
	*/

	/* if (!src.full_response.empty())
		out << YEL "Full Response..." DEF << std::endl << src.full_response << std::endl; */

	return (out);
}
