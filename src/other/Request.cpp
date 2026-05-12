#include "../../inc/requests/Request.hpp"
#include "../../inc/ansi_color_codes.h"

#include <algorithm> // transform, strtod
#include <cmath>	 // HUGE_VAL

#define URI_LIMIT 2000

Request::Request(void) { clear(); }

Request::Request(Request const &src) { *this = src; }

Request::~Request(void) {}

Request &Request::operator=(Request const &src)
{
	if (this != &src)
	{
		this->method = src.method;
		this->path_uri = src.path_uri;
		this->query = src.query;
		this->file_extension = src.file_extension;
		this->protocol = src.protocol;
		this->headers = src.headers;
		this->body = src.body;
		this->json = src.json;
		this->multi_form = src.multi_form;
		this->content_length = src.content_length;
		this->content_read = src.content_read;
		this->chuncked_body = src.chuncked_body;
		this->wanted_ranges = src.wanted_ranges;
		set_state(src.get_state());
	}
	return (*this);
}

void Request::process(std::string request)
{
	while (!request.empty())
	{
		std::cout << CYN "looping request state: " DEF << get_state() << std::endl;
		switch (get_state())
		{
		case BEGIN:
			clear();
			set_state(LINE);
			break;
		case LINE:
			parse_request_line(request);
			break;
		case HEADERS:
			parse_request_headers(request);
			break;
		case BODY:
			parse_body(request);
			break;
		default:
			request.clear();
		}
	}

	validade_request();
	std::cout << *this << std::endl;
}

void Request::clear(void)
{
	method = UNSUPPORTED_METHOD;
	path_uri.clear();
	query.clear();
	file_extension.clear();
	protocol = UNSUPPORTED_PROTOCOL;
	headers.clear();
	body.clear();
	json.clear();
	multi_form.clear();

	content_length = VALUE_NOT_SET;
	content_read = 0;

	chuncked_body = false;

	wanted_ranges.clear();

	set_state(BEGIN);
}

void Request::parse_request_line(std::string &request)
{
	size_t len;

	// -- GET METHOD
	method = HTTP::getMethod(extract(&request, " "));

	// -- GET URI
	len = request.find(" ");
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request", BAD_REQUEST));
	path_uri = request.substr(0, len);
	request.erase(0, len + 1);

	// -- GET QUERY
	len = path_uri.find("?");
	if (len != std::string::npos)
	{
		query = path_uri.substr(len + 1, path_uri.length() - len);
		path_uri.erase(len, path_uri.length() - len);
	}

	// -- GET FILE EXTENSION
	len = path_uri.rfind(".");
	if (len != std::string::npos)
	{
		file_extension = path_uri.substr(len + 1, path_uri.length() - len);
		len = file_extension.find("/");
		if (len != std::string::npos)
			file_extension.erase(len, file_extension.length() - len);
	}
	else
		file_extension = "html";

	// -- GET PROTOCOL
	protocol = HTTP::getProtocol(extract(&request, CRLF));

	if (query.size() + path_uri.size() > URI_LIMIT)
		throw(Request::ParseError("Client request URI has surpassed the Server's limit", URI_TOO_LONG));

	set_state(HEADERS);
}

void Request::parse_request_headers(std::string &request)
{
	// !! make this function less strict
	
	headers = extract_key_value(&request, ":", CRLF);
	request.erase(0, 2);
	set_state(BODY);
	update_content_length(request);
	parse_range_header();
}

size_t getMaxRequestBodySize(void)
{
	return (1000000);
}

void Request::parse_body(std::string &request)
{

	// !! Content-Length > Actual Length
	// If the Content-Length is larger than the actual length,
	// after reading to the end of the message, the server/client
	// will wait for the next byte,
	// if there's no response, it will timeout.

	// -- store body and do another loop to get missing bytes
	if (content_length - content_read > request.size())
	{
		body += request.substr(0, request.size());
		content_read += request.size();
		return (request.clear());
	}

	// !! Content-Length < Actual Length
	// If the content-length is less than the actual length,
	// the request’s message will be truncated

	// -- has enough bytes (truncates in case of too many bytes)
	body += request.substr(0, content_length - content_read);
	content_read += content_length - content_read;
	request.clear();
	
	if (body.size() > getMaxRequestBodySize())
		throw(Request::ParseError("Client body size surpassed the Server Config imposed limit", CONTENT_TOO_LARGE));

	// -- parse the body
	parse_forms();
	set_state(END);
}

void Request::validade_request(void)
{
	// !! check if method is valid for the locations in config !!
	// !! see if DELETE has body, if so PARSEERROR
	// throw(Request::ParseError("Invalid method \"" + method_names[method] + "\"" , METHOD_NOT_ALLOWED));
}

void Request::update_content_length(std::string &request)
{
	// !! if content length hasn't been registered before, update it
	// throw in case of:
	// -- the content-length is not a number
	// -- the request has body but no content-length header
	// -- AND the Transfer-Econding header is not Chuncked

	if (content_length != VALUE_NOT_SET)
		return;

	content_length = 0;
	if (headers.find("content-length") != headers.end())
	{
		char *end = NULL;
		content_length = HUGE_VAL;
		if (!headers["content-length"].empty())
			content_length = std::strtod(headers["content-length"].c_str(), &end);

		if (*end || content_length == HUGE_VAL || content_length == -HUGE_VAL)
			throw(Request::ParseError("Incorrect Content Length", BAD_REQUEST));
	}
	else if (headers.find("transfer-encoding") != headers.end())
	{
		// when the request has transfer encoding chuncked
		// request body will have to be read differently?
		if (protocol != H1_1)
			throw(Request::ParseError("Transfer-Encoding not supported for specified HTTP version", BAD_REQUEST));
		if (headers["transfer-encoding"] == "chuncked")
			chuncked_body = true;
		else
			throw(Request::ParseError("Unsupported Transfer-Encoding directive", BAD_REQUEST));
	}
	else if (request.size())
		throw(Request::ParseError("Missing required headers", LENGTH_REQUIRED));

	// there no request body, skip to end
	if (!request.size() && content_length == 0)
		return (set_state(END));
}

void Request::parse_range_header(void)
{
	if (headers.find("range") == headers.end())
		return;
	std::string value = headers["range"];
	if (value.compare(0, 6, "bytes="))
		throw(Request::ParseError("Incorrect Range header value", BAD_REQUEST));
	value.erase(0, 6);

	// Range syntax options
	// ->	<unit>=<range-start>-<range-end>
	// 		ex: 500-600
	// ->	<unit>=<range-start>-
	// 		ex: 500-
	// ->	<unit>=-<suffix-length>
	// 		ex: -600
	// ->	<unit>=<range-start>-<range-end>, …, <range-startN>-<range-endN>
	// 		ex: 500-600,700-800, 900-1000

	// std::cout << RED "STARTING RANGE HEADER PARSING" DEF << std::endl;
	// std::cout << RED "VALUE: " DEF << value << std::endl;
	while (!value.empty())
	{
		size_t sep = value.find("-");
		if (sep == std::string::npos)
		{
			wanted_ranges.clear();
			break;
		}
		// std::cout << RED "STARTING POINT: " DEF << sep << std::endl;
		// std::cout << RED "REMAINING VALUE: " DEF << value << std::endl;

		int range_start = VALUE_NOT_SET;
		int range_end = VALUE_NOT_SET;
		double tmp = HUGE_VAL;
		char *end = NULL;

		if (sep != 0)
		{
			end = NULL;
			// std::cout << RED "range start string: " DEF << value.substr(0, sep) << std::endl;
			tmp = std::strtod(value.substr(0, sep).c_str(), &end);
			if (*end || tmp == HUGE_VAL || tmp == -HUGE_VAL || tmp < 0)
			{
				wanted_ranges.clear();
				break;
			}
			range_start = static_cast<int>(tmp);
		}
		value.erase(0, sep + 1);
		sep = value.find(",");
		if (sep == std::string::npos)
			sep = value.size();
		if (sep != 0)
		{
			end = NULL;
			// std::cout << RED "range end string: " DEF << value.substr(0, sep) << std::endl;
			tmp = std::strtod(value.substr(0, sep).c_str(), &end);
			if (*end || tmp == HUGE_VAL || tmp == -HUGE_VAL || tmp < 0)
			{
				wanted_ranges.clear();
				break;
			}
			range_end = static_cast<int>(tmp);
		}
		value.erase(0, sep + 1);
		// std::cout << RED "start: " DEF << range_start << RED " end: " DEF << range_end << std::endl;
		wanted_ranges.push_back(std::pair<int, int>(range_start, range_end));
	}
	if (wanted_ranges.empty())
		throw(Request::ParseError("Incorrect Range header value", RANGE_NOT_SATISFIABLE));
}

void Request::parse_forms(void)
{
	map_strings::iterator type = headers.find("content-type");
	if (type == headers.end())
		return;
	if ((*type).second == "application/x-www-form-urlencoded")
		format_application_form();
	else if ((*type).second.find("multipart/form-data;") != std::string::npos)
		format_multipart_form((*type).second);
}

void Request::format_application_form(void)
{
	// turn body into json format...
	std::string remaining_body = body;
	map_strings json_values = extract_key_value(&remaining_body, "=", "&");
	json += "{";
	for (map_strings::iterator it = json_values.begin(); it != json_values.end();)
	{
		json += "\"" + (*it).first + "\":\"" + (*it).second + "\"";
		if (++it != json_values.end())
			json += ",";
	}
	json += "}";
	if (file_extension == "html") // to avoid overwriting other file extentions
		file_extension = "json";
}

void Request::format_multipart_form(std::string type)
{
	// -- boundary string
	// 		-- headers
	// 		-- data

	// extract boundary string from the content type header
	size_t pin = type.find("boundary=");
	if (pin == std::string::npos)
		throw(Request::ParseError("Miss formated request", BAD_REQUEST));
	std::string boundary_str = "--" + type.substr(pin + 9, type.length() - pin);

	// std::cout << RED "STARTING MULTIFORM PARSING" DEF << std::endl;
	// std::cout << RED "BOUNDARY: " DEF << boundary_str << std::endl;

	std::string remaining_body = body;
	while (!remaining_body.empty())
	{
		MultiForm part;

		pin = remaining_body.find(boundary_str);
		// std::cout << RED "STARTING POINT: " DEF << pin << std::endl;
		// std::cout << RED "REMAINING BODY: \n" DEF << remaining_body << std::endl;
		if (pin == std::string::npos)
			throw(Request::ParseError("Miss formated request", BAD_REQUEST)); // SOMETHING IS WRONG
		part.data = remaining_body.substr(pin + boundary_str.size() + 2);
		pin = part.data.find(boundary_str);
		// std::cout << RED "END: " DEF << pin << std::endl;
		if (pin == std::string::npos)
			throw(Request::ParseError("Miss formated request", BAD_REQUEST));

		part.data.erase(pin - 2, part.data.size() - pin + 4);
		remaining_body.erase(0, boundary_str.size() + 3 + part.data.size());
		// std::cout << RED "PART: \n" DEF << "|" << part.data << "|" << std::endl;

		std::string line;
		size_t end_line;
		// see if it has content-disposition
		pin = part.data.find("Content-Disposition: ");
		if (pin == std::string::npos)
			pin = part.data.find("content-disposition: ");
		if (pin != std::string::npos)
		{
			int comp = part.data.compare(pin + 21, 11, "form-data; ");
			// std::cout << "compare: " << comp << std::endl;
			if (comp)
				throw(Request::ParseError("Miss formated request", BAD_REQUEST));

			end_line = part.data.find(CRLF, pin);
			if (end_line == std::string::npos)
				throw(Request::ParseError("Miss formated request", BAD_REQUEST));
			line = part.data.substr(pin, end_line - pin);
			part.data.erase(pin, end_line + 2);
			line.erase(0, 32);
			// std::cout << RED "cont dis less: \n" DEF << part.data << std::endl;
			// std::cout << RED "cont dis line: \n" DEF << line << std::endl;
			part.content_disposition = extract_key_value(&line, "=", "; ");
		}

		// see if it has content-type
		pin = part.data.find("Content-Type: ");
		if (pin == std::string::npos)
			pin = part.data.find("content-type: ");
		if (pin != std::string::npos)
		{
			end_line = part.data.find(CRLF, pin);
			if (end_line == std::string::npos)
				throw(Request::ParseError("Miss formated request", BAD_REQUEST));
			line = part.data.substr(pin, end_line - pin);
			part.data.erase(pin, end_line + 2);
			line.erase(0, 14);
			// std::cout << RED "cont type less: \n" DEF << part.data << std::endl;
			// std::cout << RED "cont type line: \n" DEF << line << std::endl;
			part.content_type = line;
		}
		if (!part.data.compare(0, 2, CRLF))
			part.data.erase(0, 2);
		else
			throw(Request::ParseError("Miss formated request", BAD_REQUEST));

		multi_form.push_back(part);

		pin = remaining_body.find(CRLF + boundary_str + "--");
		// std::cout << RED "REMAINING BODY: \n" DEF << remaining_body << std::endl;
		// std::cout << RED "finding: " DEF << boundary_str + "--" << std::endl;
		// std::cout << RED "LASTPART: " DEF << pin << std::endl;
		if (pin == std::string::npos)
			throw(Request::ParseError("Miss formated request", BAD_REQUEST));
		if (pin == 0)
			break;
	}
}

std::string Request::extract(std::string *src, const char *sep) const
{
	size_t len = (*src).find(sep);
	if (len == std::string::npos)
		throw(Request::ParseError("Miss formated request", BAD_REQUEST));

	std::string segment = (*src).substr(0, len);
	(*src).erase(0, len + (static_cast<std::string>(sep)).length());

	return (segment);
}

map_strings Request::extract_key_value(std::string *src, std::string sep, std::string delim) const
{
	map_strings map;
	std::string key;
	std::string value;
	size_t len;
	size_t lws;

	while (!(*src).empty())
	{
		// -- get the key name
		if ((*src).find(CRLF) == 0)
			break;
		len = (*src).find(sep);
		if (len == std::string::npos)
			break;
		key = (*src).substr(0, len);
		// a header is a case-insensitive name
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		(*src).erase(0, len + sep.length());

		// -- get the value content
		len = (*src).find(delim);
		if (len == std::string::npos)
			len = (*src).size();
		lws = (*src).find_first_not_of(" \t\n\v\f\r"); // !!!!!! verify lws better
		if (lws == std::string::npos)
			lws = 0;
		value = (*src).substr(lws, len - lws);
		(*src).erase(0, len + delim.length());
		map[key] = value;
	}
	return (map);
}

void Request::set_state(t_request_state new_state) { state = new_state; }

t_request_state Request::get_state(void) const { return (state); }

std::ostream &operator<<(std::ostream &out, Request &src)
{
	out << BLU "-- Request Information --";
	out << DEF << std::endl
		<< std::endl;
	out << BLU "Method: " DEF << HTTP::stringMethod(src.method) << std::endl;
	out << BLU "URI: " DEF << src.path_uri << std::endl;
	out << BLU "file extension... " DEF << src.file_extension << std::endl;
	if (!src.query.empty())
	{
		out << BLU "    Query..." DEF << std::endl;
		out << BLU "        [" << src.query << "]" DEF << std::endl;
	}
	out << BLU "PROTOCOL: " DEF << HTTP::stringProtocol(src.protocol) << std::endl;
	out << BLU "Headers..." DEF << std::endl;
	for (map_strings::iterator it = src.headers.begin(); it != src.headers.end(); it++)
		out << BLU "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;
	if (!src.body.empty())
	{
		out << BLU "Body..." DEF << std::endl;
		out << src.body << std::endl;
		out << std::endl;
	}
	if (!src.json.empty())
	{
		out << BLU "Body formatted as JSON..." DEF << std::endl;
		out << src.json << std::endl;
		out << std::endl;
	}
	if (!src.multi_form.empty())
	{
		out << BLU "Body formatted as MultiForm..." DEF << std::endl;
		for (std::vector<MultiForm>::iterator it = src.multi_form.begin(); it != src.multi_form.end(); it++)
		{
			out << BLU "-- multi form part --";
			out << DEF << std::endl;
			out << BLU "Content Type: " DEF << (*it).content_type << std::endl;
			out << BLU "Content Disposition..." DEF << std::endl;
			for (map_strings::iterator disp = ((*it).content_disposition).begin(); disp != ((*it).content_disposition).end(); disp++)
				out << BLU "    [" << (*disp).first << "]" DEF " |" << (*disp).second << "|" << std::endl;
			out << BLU "Data..." DEF << std::endl;
			out << "|" << (*it).data << "|" << std::endl;
		}
	}
	if (!src.wanted_ranges.empty())
	{
		out << BLU "Wanted ranges from header..." DEF << std::endl;
		for (vector2::iterator it = src.wanted_ranges.begin(); it != src.wanted_ranges.end(); it++)
			out << BLU "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;
		// sleep (10);
	}
	return (out);
}
