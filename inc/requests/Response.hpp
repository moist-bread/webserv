#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

#include <stdexcept>

class Request;

// == enums

enum t_response_state
{
	PREP,
	CGI,
	METHODS,
	HEADERS_RESP,
	FULL_RESP,
	SEND
};

// =====>┊( RESPONSE )┊

class Response
{
public:
	Response(Request &src);
	Response(Response const &src);
	~Response(void);

	Response &operator=(Response const &src);

	void process(void);
	void clear(void);

	// state machine
	void set_state(t_response_state new_state);
	t_response_state get_state(void) const;

	// getters for private vars

	// private:
	void method_get(void);
	std::string create_autoindexing_page(void);
	std::string assemble_content_path(t_status_code status_code); // CAN BE AN UTILS
	std::string create_range_response_body(std::ifstream &file, vector2 &ranges);
	std::string multiple_range(std::ifstream &file, vector2 &ranges);
	std::string single_range(std::ifstream &file, std::pair<int, int> range);

	void method_post(void);
	void handle_application_form(void);
	void handle_multipart_form(void);

	void method_delete(void);

	void preparations_for_response(void);
	void execute_methods(void);
	void set_response_headers(void);
	void assemble_full_response(void);

	class CreateError : public std::runtime_error
	{
	public:
		CreateError(const std::string &msg, t_status_code status) : runtime_error("Response creation error: " + msg), response_status(status) {};
		t_status_code response_status;
	};

	Request *req;

	int file_length;
	std::string boundary;

	t_protocol protocol;
	t_status_code status_code;
	map_strings headers;
	std::string body;

	std::string cgi_reply;

	std::string full_response;

private:
	Response(void);
	t_response_state state;
};

std::ostream &operator<<(std::ostream &out, Response &src);
