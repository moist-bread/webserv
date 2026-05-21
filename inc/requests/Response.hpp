#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

#include <stdexcept>

class Request;
struct ServerConfig;

// == enums

enum t_response_state
{
	PREP,
	METHODS,
	CGI,
	CHUNK,
	HEADERS_RESP,
	FULL_RESP,
	SEND
};

// =====>┊( RESPONSE )┊

class Response
{
public:
	Response(Request &req_ref, const ServerConfig *sc);
	Response(Response const &src);
	~Response(void);
	Response &operator=(Response const &src);

	void process(void);

	void set_state(t_response_state new_state);
	t_response_state get_state(void) const;

	class CreateError : public std::runtime_error
	{
	public:
		CreateError(const std::string &msg, t_status_code status) : runtime_error("Response creation error: " + msg), response_status(status) {};
		t_status_code response_status;
	};

	int file_length;
	std::string boundary;

	t_protocol protocol;
	t_status_code status_code;
	map_strings headers;
	std::string body;

	std::string cgi_reply;
	bool is_chunked;

	std::string full_response;

private:
	Response(void);
	void clear(bool all);

	// -- response building steps
	void preparations_for_response(void);
	void execute_methods(void);
	void cgi_response(void);
	void chunk_response(void);
	void set_response_headers(void);
	void assemble_full_response(void);

	// -- method GET
	void method_get(void);
	void error_response(void);
	std::string autoindexing_page(void) const;
	std::string assemble_content_path(void);
	std::string create_range_response_body(std::ifstream &file, vector2 &ranges);
	std::string multiple_range(std::ifstream &file, vector2 &ranges);
	std::string single_range(std::ifstream &file, std::pair<int, int> range) const;

	// -- method POST
	void method_post(void);
	void handle_application_form(void);
	void handle_multipart_form(void);

	// -- method DELETE
	void method_delete(void);
	
	Request *req;
	const ServerConfig *conf;

	t_response_state state;
};

std::ostream &operator<<(std::ostream &out, Response &src);
