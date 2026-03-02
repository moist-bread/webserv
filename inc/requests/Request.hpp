#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <string>
#include <map>
#include <exception>
#include "../ansi_color_codes.h"

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


// TO-DO
//
// [ ] see if essencial headers are present
// [ ] validate headers

// =====>┊( REQUEST )┊

class Request
{
public:
	Request(void); 				// default constructor
	Request(char *rec);
	Request(Request const &source);	// copy constructor
	~Request(void);				// destructor

	Request &operator=(Request const &source); // copy assignment operator overload

	t_method method;
	std::string path_uri;
	t_protocol protocol;
	std::map<std::string, std::string> headers;

	int segment_compare_extract(std::string *src, std::string sep, std::string *cmp) const;

	class ParseError : public std::runtime_error {
		public:
			ParseError(const std::string &msg) : runtime_error("Request parse error: " + msg) {};
	};

};

std::ostream &operator<<(std::ostream &out, Request &source);
