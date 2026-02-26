#pragma once

// ==┊ needed libs by class
#include <iostream>
#include "../ansi_color_codes.h"

// == enums

enum t_method
{
	GET,
	POST,
	PUT,
	DELETE,
	PATCH
};

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
	// endpoint
	// protocol
	// host
	// ...
};

std::ostream &operator<<(std::ostream &out, Request const &source);
