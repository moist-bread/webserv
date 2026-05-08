#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

#include <stdexcept>

class Request;

// =====>┊( RESPONSE )┊

class Response
{
public:
	Response(void);
	Response(Response const &src);
	~Response(void);

	Response &operator=(Response const &src);

	void process(Request &src);
	void clear(void);

	// state machine
	void set_state(t_http_state new_state);
	t_http_state get_state(void) const;

	// getters for private vars

//private:
	void method_get(Request &src);
	std::string create_autoindexing_page(Request &src);
	static std::string assemble_content_path(Request &src, t_status_code status_code);
	std::string create_range_response_body(std::fstream &file, size_t file_len, std::string ext, vector2 &ranges);
	std::string single_range(std::fstream &file, std::pair<int, int> range);
	std::string multiple_range(std::fstream &file, size_t file_len, std::string ext, vector2 &ranges);
	
	void method_post(Request &src);
	void handle_application_form(Request &src);
	void handle_multipart_form(Request &src);

	void method_delete(Request &src);

	void set_response_headers(Request &src);
	void assemble_full_response(Request &src);

	class CreateError : public std::runtime_error
	{
	public:
		CreateError(const std::string &msg, t_status_code status) : runtime_error("Response creation error: " + msg), response_status(status) {};
		t_status_code response_status;
	};

	size_t file_length;
	std::string boundary;

	t_protocol protocol;
	t_status_code status_code;
	map_strings headers;
	std::string body;

	std::string cgi_reply;

	std::string full_response;

private:
	t_http_state state;
};

std::ostream &operator<<(std::ostream &out, Response &src);
