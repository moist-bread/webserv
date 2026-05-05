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
	Response(Response const &source);
	~Response(void);

	Response &operator=(Response const &source);

	void process(Request &src);
	void clear(Request &src);

	void method_get(Request &src);
	std::string create_autoindexing_page(Request &src);
	static std::string assemble_content_path(Request &src, t_status_code status_code);
	std::string create_range_response_body(std::ifstream &file, size_t file_len, vector2 &ranges);
	bool range_valid(size_t file_len, vector2 &ranges);
	
	std::string backup_error_page(t_status_code status_code);

	void method_post(Request &src);
	void handle_application_form(Request &src);
	void handle_multipart_form(Request &src);
	std::string random_name_generator(void) const;

	void method_delete(Request &src);

	const char *define_content_type(std::string &extension) const;
	static std::string date_generate(void);

	void eraseWritten(int start, int idx);

	class CreationError : public std::runtime_error
	{
	public:
		CreationError(const std::string &msg, t_status_code status) : runtime_error("Response creation error: " + msg), response_status(status) {};
		t_status_code response_status;
	};

	t_protocol protocol;
	t_status_code status_code;
	map_strings headers;
	std::string body;

	std::string cgi_reply;

	std::string full_response;
};

std::ostream &operator<<(std::ostream &out, Response &source);
