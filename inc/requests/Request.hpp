#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

// TO-DO
//
// [ ] DO CHUNCKED REQUESTS AND RESPONSES
// [ ] START DOING COOKIES
// [ ] adapt better for config incorporation
// [ ] vefify LWS (linear whitespace) better

// =====>┊( REQUEST )┊

class Request
{
public:
	Request(void);
	Request(Request const &source);
	~Request(void);

	Request &operator=(Request const &source);

	void process(std::string request);
	void clear(void);
	int extract_cmp_verify(std::string *src, const char *sep, std::string *cmp) const;
	map_strings extract_key_value(std::string *src, std::string sep, std::string delim) const;
	// bool detect_cgi(void) const;

	class ParseError : public std::runtime_error
	{
	public:
		ParseError(const std::string &msg, t_status_code status) : runtime_error("Request parse error: " + msg), request_status(status) {};
		t_status_code request_status;
	};

	t_method method;
	std::string path_uri;
	std::string query;
	std::string file_extension;
	t_protocol protocol;
	map_strings headers;
	std::string body;
};

std::ostream &operator<<(std::ostream &out, Request &source);
