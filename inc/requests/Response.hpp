#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

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
	static std::string assemble_content_path(Request &src, t_status_code status_code);
	static std::string get_reason_phrase(t_status_code status_code);
	static std::string date_generate(void);

	void eraseWritten(int start, int idx);

	t_protocol protocol;
	t_status_code status_code;
	map_strings headers;
	std::string body;

	std::string full_response;
};

std::ostream &operator<<(std::ostream &out, Response &source);
