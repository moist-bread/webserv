#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

#include <stdexcept>
#include <vector>

// ==┊ defines
#define CONTENT_LEN_NOT_SET -1

enum t_request_state
{
	BEGIN,
	LINE,
	HEADERS,
	BODY,
	CHUNCK,
	END
};

// TO-DO
// [x] separate response creation from request reading
// [x] only switch to POLLOUT when the reading is complete and accoring to content length
// [x] do partial REQUESTS
// [ ] do chuncked RESPONSES
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

	void parse_request_line(std::string &request);
	void parse_request_headers(std::string &request);
	void parse_body(std::string &request);
	void parse_chunck(std::string &request);
	
	void validade_request(void);
	
	void update_content_length(std::string &request);
	void parse_forms(void);
	void format_application_form(void);
	void format_multipart_form(std::string type);

	std::string extract(std::string *src, const char *sep) const;
	
	map_strings extract_key_value(std::string *src, std::string sep, std::string delim) const;

	// state machine
	void set_state(t_request_state new_state);
	t_request_state get_state(void) const;

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
	std::string json;
	std::vector<MultiForm> multi_form;
	
	double content_length;
	size_t content_read;

private:
	t_request_state state;

};

std::ostream &operator<<(std::ostream &out, Request &src);
