#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

// TO-DO
//
// [x] list of elements of an http response
// [x] add said elems to class
// [ ] implement basic file get responses
// [ ] find out what else to do


// =====>┊( RESPONSE )┊

class Response
{
public:
	Response(void);
	Response(Response const &source);
	~Response(void);

	Response &operator=(Response const &source);

	void process(Request &src, t_status_code parse_status);
	void update_response_elements(Request &src, t_status_code parse_status);
	static std::string assemble_content_path(Request &src, t_status_code status_code);
	static std::string get_reason_phrase(t_status_code status_code);
	
	t_protocol protocol;
	t_status_code status_code;
	map_strings headers;
	std::string body;

	std::string full_response;
};

std::ostream &operator<<(std::ostream &out, Response &source);
