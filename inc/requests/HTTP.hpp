#pragma once

// == libs
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <exception>
#include <stdlib.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <ctime>
#include "../ansi_color_codes.h"

// == defines

#define CRLF "\r\n"
class Request;
typedef std::map<std::string, std::string> map_strings;

struct MultiForm
{
	map_strings content_disposition;
	std::string content_type;
	std::string data;
};

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

enum t_status_code
{
	CONTINUE = 100,
	SWITCHING_PROTOCOL,
	OK = 200,
	CREATED,
	ACCEPTED,
	NON_AUTHORITATIVE_INFO,
	NO_CONTENT,
	RESET_CONTENT,
	PARTIAL_CONTENT, // in response to a Range header
	MULTIPLE_CHOICES = 300,
	MOVED_PERMANENTLY,
	FOUND,
	SEE_OTHER,
	NOT_MODIFIED,
	TEMPORARY_REDIRECT = 307,
	PERMANENT_REDIRECT,
	BAD_REQUEST = 400,
	UNAUTHORIZED,
	PAYMENT_REQUIRED,
	FORBIDDEN,
	NOT_FOUND,
	METHOD_NOT_ALLOWED,
	NOT_ACCEPTABLE,
	PROXY_AUTHENTICATION_REQUIRED,
	REQUEST_TIMEOUT,
	CONFLICT,
	GONE,
	LENGTH_REQUIRED,
	PRECONDITION_FAILED,
	REQUEST_ENTITY_TOO_LARGE,
	REQUEST_URL_TOO_LONG,
	UNSUPPORTED_MEDIA_TYPE,
	REQUESTED_RANGE_NOT_SATISFIABLE,
	EXPECTATION_FAILED, // Expect request-header
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED,
	BAD_GATEWAY,
	SERVICE_UNAVAILABLE,
	GATEWAY_TIMEOUT,
	HTTP_VERSION_NOT_SUPPORTED
};

// == template

template <typename T>
std::string to_str(const T &value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
};

// == classes

#include "Response.hpp"
#include "Request.hpp"
