#include "../../inc/requests/HTTP.hpp"

const std::string HTTP::_method_names[] = {"GET", "POST", "PUT", "DELETE", "PATCH", "UNSUPPORTED_METHOD"};
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
	for (size_t i = 0; i < number_methods;i++)
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
	case CONTINUE:
		return ("Continue");
	case SWITCHING_PROTOCOL:
		return ("Switching Protocols");
	case OK:
		return ("OK");
	case CREATED:
		return ("Created");
	case ACCEPTED:
		return ("Accepted");
	case NON_AUTHORITATIVE_INFO:
		return ("Non-authoritative Information");
	case NO_CONTENT:
		return ("No Content");
	case RESET_CONTENT:
		return ("Reset Content");
	case PARTIAL_CONTENT:
		return ("Partial Content");
	case MULTIPLE_CHOICES:
		return ("Multiple Choices");
	case MOVED_PERMANENTLY:
		return ("Moved Permanently");
	case FOUND:
		return ("Found");
	case SEE_OTHER:
		return ("See Other");
	case NOT_MODIFIED:
		return ("Not Modified");
	case TEMPORARY_REDIRECT:
		return ("Temporary Redirect");
	case PERMANENT_REDIRECT:
		return ("Permanent Redirect");
	case BAD_REQUEST:
		return ("Bad Request");
	case UNAUTHORIZED:
		return ("Unauthorized");
	case PAYMENT_REQUIRED:
		return ("Payment Required");
	case FORBIDDEN:
		return ("Forbidden");
	case NOT_FOUND:
		return ("Not Found");
	case METHOD_NOT_ALLOWED:
		return ("Method Not Allowed");
	case NOT_ACCEPTABLE:
		return ("Not Acceptable");
	case PROXY_AUTHENTICATION_REQUIRED:
		return ("Proxy Authentication Required");
	case REQUEST_TIMEOUT:
		return ("Request Timeout");
	case CONFLICT:
		return ("Conflict");
	case GONE:
		return ("Gone");
	case LENGTH_REQUIRED:
		return ("Length Required");
	case PRECONDITION_FAILED:
		return ("Precondition Failed");
	case REQUEST_ENTITY_TOO_LARGE:
		return ("Request Entity Too Large");
	case REQUEST_URL_TOO_LONG:
		return ("Request-url Too Long");
	case UNSUPPORTED_MEDIA_TYPE:
		return ("Unsupported Media Type");
	case REQUESTED_RANGE_NOT_SATISFIABLE:
		return ("Requested Range Not Satisfiable");
	case EXPECTATION_FAILED:
		return ("Expectation Failed");
	case INTERNAL_SERVER_ERROR:
		return ("Internal Server Error");
	case NOT_IMPLEMENTED:
		return ("Not Implemented");
	case BAD_GATEWAY:
		return ("Bad Gateway");
	case SERVICE_UNAVAILABLE:
		return ("Service Unavailable");
	case GATEWAY_TIMEOUT:
		return ("Gateway Timeout");
	case HTTP_VERSION_NOT_SUPPORTED:
		return ("HTTP Version Not Supported");
	default:
		return ("OK");
	}
}
