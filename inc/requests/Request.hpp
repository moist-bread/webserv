#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

#include <stdexcept>

// TO-DO
// [+-] incorporation config into response
// [+-] incorporation config into request
// [+-] incorporation config into cgi
// [ ]  vefify LWS (linear whitespace) better
// [ ]  make the gallery work like the boardby going by the database file
// [x]  switch strtod for strtol (so it doesnt accept 1.1 values)
// [ ]  create a static Inspect class for debug prints
// [ ]  ?? do i need to accept transfer encoding chuncked REQUESTS?

struct ServerConfig;
struct LocationConfig;

// =====>┊( REQUEST )┊

class Request
{
public:
	Request(const ServerConfig *sc);
	Request(Request const &src);
	~Request(void);

	Request &operator=(Request const &src);

	void process(std::string request);

	void set_state(const t_request_state &new_state);
	const t_request_state &get_state(void) const;

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
	vector2 wanted_ranges;

	const ServerConfig *conf;
	const LocationConfig *loc;

private:
	Request(void);

	void clear(void);

	void parse_request_line(std::string &request);
	void parse_request_headers(std::string &request);
	void parse_body(std::string &request);

	void update_content_length(std::string &request);
	void parse_range_header(void);
	void parse_forms(void);
	void format_application_form(void);
	void format_multipart_form(const std::string &type);

	void validade_request(void);
	std::string extract(std::string *src, const char *sep) const;

	t_request_state state;

	std::string temp_headers;
	long content_length;
	size_t content_read;
	// bool chunked_body;
};

std::ostream &operator<<(std::ostream &out, Request &src);
