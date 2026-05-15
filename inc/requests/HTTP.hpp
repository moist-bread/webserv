#pragma once

// == libs
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <stdio.h>
#include <stdint.h> // size_max

// == defines
#define CRLF "\r\n"
#define VALUE_NOT_SET -1

typedef std::map<std::string, std::string> map_strings;
typedef std::vector<std::pair<int, int> > vector2;

struct MultiForm
{
	map_strings content_disposition;
	std::string content_type;
	std::string data;
};

// == enums

enum t_request_state
{
	BEGIN,
	LINE,
	HEADERS,
	BODY,
	END
};

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
	CONTENT_TOO_LARGE,
	URI_TOO_LONG,
	UNSUPPORTED_MEDIA_TYPE,
	RANGE_NOT_SATISFIABLE,
	EXPECTATION_FAILED, // Expect request-header
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED,
	BAD_GATEWAY,
	SERVICE_UNAVAILABLE,
	GATEWAY_TIMEOUT,
	HTTP_VERSION_NOT_SUPPORTED
};

// == template
#include <sstream>

template <typename T>
std::string to_str(const T &value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
};

// == classes
class HTTP
{
public:
	static t_method getMethod(const std::string &strMethod);	   // returns the Method Enum from string
	static std::string stringMethod(const t_method Method);		   // returns the string from Method Enum
	static t_protocol getProtocol(const std::string &strProtocol); // returns the Protocol Enum from string
	static std::string stringProtocol(const t_protocol Protocol);  // returns the string from Protocol Enum
	static std::string getReasonPhrase(t_status_code status_code); // returns the string from Status Code Enum
	static map_strings extract_key_value(std::string *src, std::string sep, std::string delim); 
	 

private:
	HTTP(void);				  // default constructor
	HTTP(HTTP const &source); // copy constructor
	~HTTP(void);			  // destructor

	HTTP &operator=(HTTP const &source); // copy assignment operator overload

	static const std::string _method_names[];
	static const std::string _protocol_names[];
};
