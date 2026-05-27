#include "../../inc/requests/Request.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include "../../inc/string_utils.tpp"

#include "../../inc/ansi_color_codes.h"

#include <algorithm> // strtol
#include <climits>	 // LONG_MAX

#define URI_LIMIT 2000

Request::Request(void) : conf(NULL) { clear(); }

Request::Request(const ServerConfig *sc) : conf(sc) { clear(); }

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
		this->wanted_ranges = src.wanted_ranges;

		this->loc = src.loc;

		set_state(src.get_state());

		this->temp_str = src.temp_str;
		this->content_length = src.content_length;
		this->content_read = src.content_read;
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
		case HEADERS_REQ:
			parse_request_headers(request);
			break;
		case CHUNK_BODY:
			parse_chunk(request);
			break;
		case BODY:
			parse_body(request);
			break;
		default:
			request.clear();
		}
	}
	std::cout << *this << std::endl;
	if (state == END)
		validade_request();
}

void Request::set_state(const t_request_state &new_state) { state = new_state; }

const t_request_state &Request::get_state(void) const { return (state); }

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
	wanted_ranges.clear();

	loc = NULL;

	set_state(BEGIN);

	temp_str.clear();
	content_length = VALUE_NOT_SET;
	content_read = 0;
}

void Request::parse_request_line(std::string &request)
{

	if (!temp_str.empty())
	{
		request.insert(0, temp_str);
		temp_str.clear();
	}

	// see if the end of the request line is present or not
	// if not: keep waiting for for requets (until timeout if needed)
	if (request.find(CRLF) == std::string::npos)
	{
		temp_str = request;
		return (request.clear());
	}

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

	set_state(HEADERS_REQ);
}

void Request::parse_request_headers(std::string &request)
{
	if (!temp_str.empty())
	{
		request.insert(0, temp_str);
		temp_str.clear();
	}

	// see if the end of the header already exists or not
	// if not: keep waiting for for requets (until timeout if needed)
	if (request.find(CRLF CRLF) == std::string::npos)
	{
		temp_str = request;
		return (request.clear());
	}

	headers = HTTP::extract_key_value(&request, ":", CRLF);
	request.erase(0, 2);
	set_state(BODY);
	update_content_length(request);
	parse_range_header();
}

void Request::parse_body(std::string &request)
{

	if (body.size() > conf->getClientMaxBodySize())
		throw(Request::ParseError("Client body size surpassed the Server Config imposed limit", CONTENT_TOO_LARGE));

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

	// -- parse the body
	parse_forms();
	set_state(END);
}

void Request::parse_chunk(std::string &request)
{
	if (!temp_str.empty())
	{
		request.insert(0, temp_str);
		temp_str.clear();
	}

	while (!request.empty())
	{
		size_t pin;

		// std::cout << YEL "actual request rn:" DEF << std::endl;
		// std::cout << request << std::endl;
		// if (content_length == VALUE_NOT_SET)
		// 	std::cout << YEL "size of chunck data not yet set..." DEF << std::endl;
		
		// -- step 1: wait until you have a CRLF
		pin = request.find(CRLF);
		if (pin == std::string::npos)
		{
			temp_str = request;
			request.clear();
		}
		else if (content_length == VALUE_NOT_SET)
		{
			// -- step 2: save the hex number as your content length for the chunck
			try
			{
				std::cout << YEL "getting size of chunck data..." DEF << std::endl;
				content_length = stringToNumber<long>(request.substr(0, pin), std::hex);
				request.erase(0, pin + 2);
			}
			catch(const std::exception& e)
			{
				throw(Request::ParseError("Invalid Chunck size of the chunk-data", BAD_REQUEST));
			}
		}
		else
		{
			// -- step 3: wait until the chunk ending CRLF
			body += request.substr(0, pin);
			request.erase(0, pin + 2);
			if (content_length == 0)
			{
				std::cout << YEL "finishing chunk has been processed !!!" DEF << std::endl;
				return (set_state(END), request.clear());
			}
			else
			{
				std::cout << YEL "one chunk done, onto the next..." DEF << std::endl;
				content_length = VALUE_NOT_SET;
			}
		}
	}
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
		// --------------------- change to string_to_number
		char *end = NULL;
		content_length = LONG_MAX;
		if (!headers["content-length"].empty())
			content_length = std::strtol(headers["content-length"].c_str(), &end, 10);

		if (*end || content_length == LONG_MAX || content_length < 0)
			throw(Request::ParseError("Incorrect Content Length", BAD_REQUEST));
	}
	else if (headers.find("transfer-encoding") != headers.end())
	{
		content_length = VALUE_NOT_SET;
		if (protocol != H1_1)
			throw(Request::ParseError("Transfer-Encoding not supported for specified HTTP version", BAD_REQUEST));
		if (headers["transfer-encoding"] == "chunked")
			return (set_state(CHUNK_BODY));
		else
			throw(Request::ParseError("Unsupported Transfer-Encoding directive", BAD_REQUEST));
	}
	else if (request.size())
		throw(Request::ParseError("Missing required headers", LENGTH_REQUIRED));

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

	while (!value.empty())
	{
		size_t sep = value.find("-");
		if (sep == std::string::npos)
		{
			wanted_ranges.clear();
			break;
		}

		int range_start = VALUE_NOT_SET;
		int range_end = VALUE_NOT_SET;
		long tmp = LONG_MAX;
		char *end = NULL;

		if (sep != 0)
		{
			// --------------------- change to string_to_number
			end = NULL;
			tmp = std::strtol(value.substr(0, sep).c_str(), &end, 10);
			if (*end || tmp > INT_MAX || tmp < 0)
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
			// --------------------- change to string_to_number
			end = NULL;
			tmp = std::strtol(value.substr(0, sep).c_str(), &end, 10);
			if (*end || tmp > INT_MAX || tmp < 0)
			{
				wanted_ranges.clear();
				break;
			}
			range_end = static_cast<int>(tmp);
		}
		value.erase(0, sep + 1);
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
	map_strings json_values = HTTP::extract_key_value(&remaining_body, "=", "&");
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

void Request::format_multipart_form(const std::string &type)
{
	// -- boundary string
	// 		-- headers
	// 		-- data

	// extract boundary string from the content type header
	size_t pin = type.find("boundary=");
	if (pin == std::string::npos)
		throw(Request::ParseError("Miss formated request set bound", BAD_REQUEST));
	std::string boundary_str = "--" + type.substr(pin + 9, type.length() - pin);

	std::string remaining_body = body;
	while (!remaining_body.empty())
	{
		MultiForm part;

		pin = remaining_body.find(boundary_str);
		if (pin == std::string::npos)
			throw(Request::ParseError("Miss formated request skip first bound", BAD_REQUEST));
		part.data = remaining_body.substr(pin + boundary_str.size() + 2);
		pin = part.data.find(boundary_str);
		if (pin == std::string::npos)
			throw(Request::ParseError("Miss formated request find next bound", BAD_REQUEST));

		part.data.erase(pin - 2, part.data.size() - pin + 4);
		remaining_body.erase(0, (!remaining_body.compare(0, 2, CRLF) ? 2 : 0) + boundary_str.size() + 2 + part.data.size());

		std::string line;
		size_t end_line;
		// see if it has content-disposition
		pin = part.data.find("Content-Disposition: ");
		if (pin == std::string::npos)
			pin = part.data.find("content-disposition: ");
		if (pin != std::string::npos)
		{
			int comp = part.data.compare(pin + 21, 11, "form-data; ");
			if (comp)
				throw(Request::ParseError("Miss formated request form data", BAD_REQUEST));

			end_line = part.data.find(CRLF, pin);
			if (end_line == std::string::npos)
				throw(Request::ParseError("Miss formated request end of cont disp", BAD_REQUEST));
			line = part.data.substr(pin, end_line - pin);
			part.data.erase(pin, end_line + 2);
			line.erase(0, 32);
			part.content_disposition = HTTP::extract_key_value(&line, "=", "; ");
		}

		// see if it has content-type
		pin = part.data.find("Content-Type: ");
		if (pin == std::string::npos)
			pin = part.data.find("content-type: ");
		if (pin != std::string::npos)
		{
			end_line = part.data.find(CRLF, pin);
			if (end_line == std::string::npos)
				throw(Request::ParseError("Miss formated request end of cont type", BAD_REQUEST));
			line = part.data.substr(pin, end_line - pin);
			part.data.erase(pin, end_line + 2);
			line.erase(0, 14);
			part.content_type = line;
		}
		if (!part.data.compare(0, 2, CRLF))
			part.data.erase(0, 2);
		else
			throw(Request::ParseError("Miss formated request empty line after headers", BAD_REQUEST));

		multi_form.push_back(part);

		pin = remaining_body.find(CRLF + boundary_str + "--");
		if (pin == std::string::npos)
			throw(Request::ParseError("Miss formated request closing bound", BAD_REQUEST));
		if (pin == 0)
			break;
	}
}

void Request::validade_request(void)
{
	if (method == DELETE && !body.empty())
		throw(Request::ParseError("Requests with body are not supported by this server", BAD_REQUEST));

	loc = conf->matchLocation(path_uri);
	if (!loc)
		throw(Request::ParseError("The page you're trying to acess does not exists", NOT_FOUND));
	std::cout << RED "CURRENT LOCATION" DEF << std::endl;
	std::cout << *loc << std::endl;

	if (!loc->isMethodAllowed(method))
		throw(Request::ParseError("Invalid method \"" + HTTP::stringMethod(method) + "\"", METHOD_NOT_ALLOWED));
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
	/*
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
	*/
	if (!src.wanted_ranges.empty())
	{
		out << BLU "Wanted ranges from header..." DEF << std::endl;
		for (vector2::iterator it = src.wanted_ranges.begin(); it != src.wanted_ranges.end(); it++)
			out << BLU "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;
	}
	return (out);
}
