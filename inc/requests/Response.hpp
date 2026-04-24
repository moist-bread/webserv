#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>

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
	std::string backup_error_page(t_status_code status_code);

	void method_post(Request &src);
	void handle_application_form(Request &src);
	void handle_multipart_form(Request &src);
	std::string random_name_generator(void) const;

	void method_delete(Request &src);

	const char *define_content_type(std::string &extension) const;
	static std::string date_generate(void);

	void eraseWritten(int start, int idx);

	t_protocol protocol;
	t_status_code status_code;
	map_strings headers;
	std::string body;

	std::string full_response;
};

std::ostream &operator<<(std::ostream &out, Response &source);
