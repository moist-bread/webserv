#include "../../inc/requests/Request.hpp"
#include "../../inc/ansi_color_codes.h"

#include <algorithm>	// transform, strtod
#include <cmath>		// HUGE_VAL

Request::Request(void) { clear(); }

Request::Request(Request const &source) { *this = source; }

Request::~Request(void) {}

Request &Request::operator=(Request const &source)
{
	if (this != &source)
	{
		this->method = source.method;
		this->path_uri = source.path_uri;
		this->query = source.query;
		this->file_extension = source.file_extension;
		this->protocol = source.protocol;
		this->headers = source.headers;
		this->body = source.body;
		this->json = source.json;
		this->multi_form = source.multi_form;
		this->missing_request_part = source.missing_request_part;
		this->content_length = source.content_length;
		this->content_read = source.content_read;
		this->state = source.state;
	}
	return (*this);
}

void Request::process(std::string request)
{
	// while request !empty()
	// switchcase state
	if (!missing_request_part)
	{
		clear();
		parse_request_line(request);
		state = HEADERS;
		headers = extract_key_value(&request, ":", CRLF);
		request.erase(0, 2);
	}
	parse_body(request);

	// !! check if method is valid for the locations in config !!
	// !! see if DELETE has body, if so PARSEERROR
	// throw(Request::ParseError("Invalid method \"" + method_names[method] + "\"" , METHOD_NOT_ALLOWED));

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

	missing_request_part = false;

	content_length = CONTENT_LEN_NOT_SET;
	content_read = 0;
	state = BEGIN;
}

void Request::parse_request_line(std::string &request)
{
	size_t len;

	state = LINE;
	
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
}

void Request::parse_body(std::string &request)
{
	state = BODY;

	// -- GET CONTENT LENGTH/CHUNCK
	update_content_length(request);

	// !! Content-Length > Actual Length
	// If the Content-Length is larger than the actual length,
	// after reading to the end of the message, the server/client
	// will wait for the next byte,
	// naturally leading to no response until timeout.

	// !! Content-Length < Actual Length
	// If this length is less than the actual length,
	// the first request’s message will be truncated

	// -- how to do it
	// - have a state of parse to know where it was left off
	// - store the amount of read_bytes to be able to keep track
	// - when theres no content length guide by Transfer-Encoding: chunked
	
	
	// Cache-Control: max-age=0
	if (content_length != request.size() && headers.find("cache-control") != headers.end() && headers["cache-control"] == "max-age=0")
	{
		std::cout << "remaining request: " << request.size() << " actual len: " << content_length << std::endl;
		missing_request_part = true; // -- THROW SOMETHING SPECIFIC FOR THIS
		return ;
	}
	else if (content_length != request.size())
	{

		std::cout << "remaining request: " << request.size() << " actual len: " << content_length << std::endl;
		throw(Request::ParseError("Incorrect Content Length", BAD_REQUEST));
	}
	
	// -- GET BODY
	body = request.substr(0, request.size());
	parse_forms();	
	missing_request_part = false;
	state = END;
}

void Request::update_content_length(std::string &request)
{
	// if content length hasn't been read before, update it
	// throw in case of:
	//	-> the content-length is not a number
	//	-> the request has length but no content-length header
	//     AND the Transfer-Econding header is not Chuncked
	
	if (content_length != CONTENT_LEN_NOT_SET)
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
	else if (request.size())
	{
		if (headers.find("transfer-encoding") != headers.end())
		{
			std::string encoding = headers["transfer-encoding"];
			std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::tolower);
			if (encoding == "chuncked")
			{
				state = CHUNCK;
				return;
			}
		}
		throw(Request::ParseError("Missing required headers", LENGTH_REQUIRED));
	}
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


void Request::format_application_form (void)
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
	if (file_extension == "html") // to avoid overwriting other f_ext
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
		throw (Request::ParseError("Miss formated request", BAD_REQUEST));
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
			throw (Request::ParseError("Miss formated request", BAD_REQUEST)); // SOMETHING IS WRONG
		part.data = remaining_body.substr(pin + boundary_str.size() + 2);
		pin = part.data.find(boundary_str);
		// std::cout << RED "END: " DEF << pin << std::endl;
		if (pin == std::string::npos)
			throw (Request::ParseError("Miss formated request", BAD_REQUEST));
		
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
			int comp = part.data.compare(pin + 21, 11,"form-data; ");
			// std::cout << "compare: " << comp << std::endl;
			if (comp)
				throw (Request::ParseError("Miss formated request", BAD_REQUEST));
			
			end_line = part.data.find(CRLF, pin);
			if (end_line == std::string::npos)
				throw (Request::ParseError("Miss formated request", BAD_REQUEST));
			line = part.data.substr(pin, end_line - pin);
			part.data.erase(pin, end_line + 2);
			line.erase(0, 32);
			//std::cout << RED "cont dis less: \n" DEF << part.data << std::endl;
			//std::cout << RED "cont dis line: \n" DEF << line << std::endl;
			part.content_disposition = extract_key_value(&line, "=", "; " );
		}

		// see if it has content-type
		pin = part.data.find("Content-Type: ");
		if (pin == std::string::npos)
			pin = part.data.find("content-type: ");
		if (pin != std::string::npos)
		{
			end_line = part.data.find(CRLF, pin);
			if (end_line == std::string::npos)
				throw (Request::ParseError("Miss formated request", BAD_REQUEST));
			line = part.data.substr(pin, end_line - pin);
			part.data.erase(pin, end_line + 2);
			line.erase(0, 14);
			//std::cout << RED "cont type less: \n" DEF << part.data << std::endl;
			//std::cout << RED "cont type line: \n" DEF << line << std::endl;
			part.content_type = line;
		}
		if (!part.data.compare(0, 2, CRLF))
			part.data.erase(0, 2);
		else
			throw (Request::ParseError("Miss formated request", BAD_REQUEST));

		multi_form.push_back(part);

		pin = remaining_body.find(CRLF + boundary_str + "--");
		// std::cout << RED "REMAINING BODY: \n" DEF << remaining_body << std::endl;
		// std::cout << RED "finding: " DEF << boundary_str + "--" << std::endl;
		// std::cout << RED "LASTPART: " DEF << pin << std::endl;
		if (pin == std::string::npos)
			throw (Request::ParseError("Miss formated request", BAD_REQUEST));
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

	//	* PREVIOUS VERSION *
	// for (int i = 0; !cmp[i].empty(); i++)
	// 	if (!segment.compare(cmp[i]))
	// 		return (i);
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

std::ostream &operator<<(std::ostream &out, Request &src)
{
	out << BLU "-- Request Information --";
	out << DEF << std::endl
		<< std::endl;
	out << BLU "Method: " DEF << HTTP::stringMethod(src.method) << std::endl;
	out << BLU "URI: " DEF << src.path_uri << std::endl;
	out << BLU "file extension... " DEF << src.file_extension << std::endl;
	out << BLU "missing request part... " DEF << src.missing_request_part << std::endl;
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
	return (out);
}
