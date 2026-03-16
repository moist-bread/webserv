#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

// TO-DO
//
// [ ] see if essencial headers are present
// [ ] validate headers
// [x] make parsing work for POST
// [ ] vefify LWS (linear whitespace) better

// =====>┊( REQUEST )┊

class Request
{
public:
	Request(void);
	Request(Request const &source);
	~Request(void);
	
	Request &operator=(Request const &source);
	
	void process(char *rec);
	int extract_cmp_verify(std::string *src, const char *sep, std::string *cmp) const;
	map_strings extract_key_value(std::string *src, std::string sep, std::string delim) const;

	class ParseError : public std::runtime_error {
		public:
			ParseError(const std::string &msg, t_status_code status) : runtime_error("Request parse error: " + msg), request_status(status) {}; // 400 Bad Request
			t_status_code request_status;
	};

	t_method method;
	std::string path_uri;
	map_strings query;
	t_protocol protocol;
	map_strings headers;
	map_strings body;

};

std::ostream &operator<<(std::ostream &out, Request &source);
