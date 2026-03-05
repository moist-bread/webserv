#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <string>
#include <map>
#include <exception>
#include "../ansi_color_codes.h"

#define CRLF "\r\n"

// == enums

enum t_method
{
	GET,
	POST,
	PUT,
	DELETE,
	PATCH,
	UNSUPPORTED_METHOD
};

enum t_protocol
{
	H1_0,
	H1_1,
	UNSUPPORTED_PROTOCOL
};


typedef std::map<std::string, std::string>	map_strings;

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
	Request(char *rec);
	Request(Request const &source);
	~Request(void);

	Request &operator=(Request const &source); // copy assignment operator overload

	int extract_cmp_verify(std::string *src, const char *sep, std::string *cmp) const;
	map_strings extract_key_value(std::string *src, std::string sep, std::string delim) const;

	class ParseError : public std::runtime_error {
		public:
			ParseError(const std::string &msg) : runtime_error("Request parse error: " + msg) {};
	};

	t_method method;
	std::string path_uri;
	map_strings query;
	t_protocol protocol;
	map_strings headers;
	map_strings body;

};

std::ostream &operator<<(std::ostream &out, Request &source);
