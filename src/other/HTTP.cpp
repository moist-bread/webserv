#include "../../inc/requests/HTTP.hpp"

#include <algorithm> // transform

const std::string HTTP::_method_names[] = {"GET", "POST", "PUT", "DELETE", "HEAD", "PATCH", "UNSUPPORTED_METHOD"};
const std::string HTTP::_protocol_names[] = {"HTTP/1.0", "HTTP/1.1", "UNSUPPORTED_PROTOCOL"};

HTTP::HTTP(void) {}

HTTP::HTTP(HTTP const &source) { *this = source; }

HTTP::~HTTP(void) {}

HTTP &HTTP::operator=(HTTP const &source)
{
	if (this != &source)
		(void)source;
	return (*this);
}

t_method HTTP::getMethod(const std::string &strMethod)
{
	size_t number_methods = (sizeof(_method_names) / sizeof(_method_names[0]));
	for (size_t i = 0; i < number_methods; i++)
	{
		if (!strMethod.compare(_method_names[i]))
			return (static_cast<t_method>(i));
	}
	return (UNSUPPORTED_METHOD);
}

std::string HTTP::stringMethod(const t_method Method)
{
	return (_method_names[Method]);
}

t_protocol HTTP::getProtocol(const std::string &strProtocol)
{
	size_t number_protocols = (sizeof(_protocol_names) / sizeof(_protocol_names[0]));
	for (size_t i = 0; i < number_protocols; i++)
	{
		if (!strProtocol.compare(_protocol_names[i]))
			return (static_cast<t_protocol>(i));
	}
	return (UNSUPPORTED_PROTOCOL);
}

std::string HTTP::stringProtocol(const t_protocol Protocol)
{
	return (_protocol_names[Protocol]);
}

std::string HTTP::getReasonPhrase(t_status_code status_code)
{
	switch (status_code)
	{
		case CONTINUE:						return ("Continue");
		case SWITCHING_PROTOCOL:			return ("Switching Protocols");
		case OK:							return ("OK");
		case CREATED:						return ("Created");
		case ACCEPTED:						return ("Accepted");
		case NON_AUTHORITATIVE_INFO:		return ("Non-authoritative Information");
		case NO_CONTENT:					return ("No Content");
		case RESET_CONTENT:					return ("Reset Content");
		case PARTIAL_CONTENT:				return ("Partial Content");
		case MULTIPLE_CHOICES:				return ("Multiple Choices");
		case MOVED_PERMANENTLY:				return ("Moved Permanently");
		case FOUND:							return ("Found");
		case SEE_OTHER:						return ("See Other");
		case NOT_MODIFIED:					return ("Not Modified");
		case TEMPORARY_REDIRECT:			return ("Temporary Redirect");
		case PERMANENT_REDIRECT:			return ("Permanent Redirect");
		case BAD_REQUEST:					return ("Bad Request");
		case UNAUTHORIZED:					return ("Unauthorized");
		case PAYMENT_REQUIRED:				return ("Payment Required");
		case FORBIDDEN:						return ("Forbidden");
		case NOT_FOUND:						return ("Not Found");
		case METHOD_NOT_ALLOWED:			return ("Method Not Allowed");
		case NOT_ACCEPTABLE:				return ("Not Acceptable");
		case PROXY_AUTHENTICATION_REQUIRED:	return ("Proxy Authentication Required");
		case REQUEST_TIMEOUT:				return ("Request Timeout");
		case CONFLICT:						return ("Conflict");
		case GONE:							return ("Gone");
		case LENGTH_REQUIRED:				return ("Length Required");
		case PRECONDITION_FAILED:			return ("Precondition Failed");
		case CONTENT_TOO_LARGE:				return ("Content Too Large");
		case URI_TOO_LONG:					return ("URI Too Long");
		case UNSUPPORTED_MEDIA_TYPE:		return ("Unsupported Media Type");
		case RANGE_NOT_SATISFIABLE:			return ("Range Not Satisfiable");
		case EXPECTATION_FAILED:			return ("Expectation Failed");
		case INTERNAL_SERVER_ERROR:			return ("Internal Server Error");
		case NOT_IMPLEMENTED:				return ("Not Implemented");
		case BAD_GATEWAY:					return ("Bad Gateway");
		case SERVICE_UNAVAILABLE:			return ("Service Unavailable");
		case GATEWAY_TIMEOUT:				return ("Gateway Timeout");
		case HTTP_VERSION_NOT_SUPPORTED:	return ("HTTP Version Not Supported");
		default:							return ("OK");
	}
}

t_status_code HTTP::isValidReasonPhrase(const int status_code)
{
	switch (status_code)
	{
		case CONTINUE:						return (CONTINUE);
		case SWITCHING_PROTOCOL:			return (SWITCHING_PROTOCOL);
		case OK:							return (OK);
		case CREATED:						return (CREATED);
		case ACCEPTED:						return (ACCEPTED);
		case NON_AUTHORITATIVE_INFO:		return (NON_AUTHORITATIVE_INFO);
		case NO_CONTENT:					return (NO_CONTENT);
		case RESET_CONTENT:					return (RESET_CONTENT);
		case PARTIAL_CONTENT:				return (PARTIAL_CONTENT);
		case MULTIPLE_CHOICES:				return (MULTIPLE_CHOICES);
		case MOVED_PERMANENTLY:				return (MOVED_PERMANENTLY);
		case FOUND:							return (FOUND);
		case SEE_OTHER:						return (SEE_OTHER);
		case NOT_MODIFIED:					return (NOT_MODIFIED);
		case TEMPORARY_REDIRECT:			return (TEMPORARY_REDIRECT);
		case PERMANENT_REDIRECT:			return (PERMANENT_REDIRECT);
		case BAD_REQUEST:					return (BAD_REQUEST);
		case UNAUTHORIZED:					return (UNAUTHORIZED);
		case PAYMENT_REQUIRED:				return (PAYMENT_REQUIRED);
		case FORBIDDEN:						return (FORBIDDEN);
		case NOT_FOUND:						return (NOT_FOUND);
		case METHOD_NOT_ALLOWED:			return (METHOD_NOT_ALLOWED);
		case NOT_ACCEPTABLE:				return (NOT_ACCEPTABLE);
		case PROXY_AUTHENTICATION_REQUIRED:	return (PROXY_AUTHENTICATION_REQUIRED);
		case REQUEST_TIMEOUT:				return (REQUEST_TIMEOUT);
		case CONFLICT:						return (CONFLICT);
		case GONE:							return (GONE);
		case LENGTH_REQUIRED:				return (LENGTH_REQUIRED);
		case PRECONDITION_FAILED:			return (PRECONDITION_FAILED);
		case CONTENT_TOO_LARGE:				return (CONTENT_TOO_LARGE);
		case URI_TOO_LONG:					return (URI_TOO_LONG);
		case UNSUPPORTED_MEDIA_TYPE:		return (UNSUPPORTED_MEDIA_TYPE);
		case RANGE_NOT_SATISFIABLE:			return (RANGE_NOT_SATISFIABLE);
		case EXPECTATION_FAILED:			return (EXPECTATION_FAILED);
		case INTERNAL_SERVER_ERROR:			return (INTERNAL_SERVER_ERROR);
		case NOT_IMPLEMENTED:				return (NOT_IMPLEMENTED);
		case BAD_GATEWAY:					return (BAD_GATEWAY);
		case SERVICE_UNAVAILABLE:			return (SERVICE_UNAVAILABLE);
		case GATEWAY_TIMEOUT:				return (GATEWAY_TIMEOUT);
		case HTTP_VERSION_NOT_SUPPORTED:	return (HTTP_VERSION_NOT_SUPPORTED);
		default:							return (INVALID_CODE);
	}
}

map_strings HTTP::extract_key_value(std::string *str, std::string sep, std::string delim)
{
	map_strings map;
	std::string key;
	std::string value;
	size_t len;

	// LWS = [CRLF] 1*( SP | HT )
	// (translation: if there's a new line and a space/tab after is counts as a space)
	
	// header field values can be folded onto multiple lines if the
	// continuation line begins with a space or horizontal tab

	// Each header field consists of a name followed by a colon (":")
	// and the field value. Field names are case-insensitive.
	
	// The field value MAY be preceded by any amount of LWS, though a single SP is preferred.

	while (!(*str).empty())
	{
		// -- get the key name
		if ((*str).find(CRLF) == 0)
			break;
		len = (*str).find(sep);
		if (len == std::string::npos)
			break;
		key = (*str).substr(0, len);
		// a header field name is case-insensitive
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		(*str).erase(0, len + sep.length());

		// -- skip whitespaces/LWS
		if ((*str).find(CRLF SP) == 0 || (*str).find(CRLF HT) == 0)
			len = 3;
		else if ((*str).find(SP) == 0)
			len = 1;
		else
			len = 0;
		(*str).erase(0, len);
		
		// -- get the value content
		len = (*str).find(delim);
		if (len == std::string::npos)
			len = (*str).size();
		
		value = (*str).substr(0, len);
		(*str).erase(0, len + delim.length());
		map[key] = value;
	}
	return (map);
}
