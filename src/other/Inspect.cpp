#include "../../inc/requests/Inspect.hpp"

#include "../../inc/requests/HTTP.hpp"
#include "../../inc/sockets/Server.hpp"
#include "../../inc/requests/Request.hpp"
#include "../../inc/requests/Response.hpp"

#include "../../inc/ansi_color_codes.h"

#include <ctime>	// time, localtime, strftime

const bool Inspect::debug = static_cast<bool>(DEBUG);

void Inspect::inspect_server_activity(const std::string &msg, const Server &sv)
{
	write_curr_time();

	std::cout << MAG "Server has " << msg << "; " << DEF << std::endl;
	if (debug && msg == "started up")
		std::cout << sv << std::endl;
}

void Inspect::inspect_client_activity(const std::string &msg, const int &fd, const int &port)
{
	write_curr_time();

	// ex.: Client 5 has been accepted; Port=9090
	std::cout << CYN "Client " << fd << " has " << msg << "; ";
	std::cout << "Port=" << port << DEF << std::endl;
	
}

void Inspect::inspect_request_activity(const std::string &msg, const Request &req)
{
	write_curr_time();

	// ex.: Request has been fully recieved; GET / HTTP/1.1
	if (msg.empty())
	{
		std::cout << BLU "Request has been fully recieved; ";
		std::cout << HTTP::stringMethod(req.method) << " ";
		std::cout << req.path_uri << " ";
		std::cout << HTTP::stringProtocol(req.protocol) << DEF << std::endl;
	}
	else
		std::cerr << BLU "Request has encountered an error; " << msg << DEF << std::endl;
	if (debug)
		std::cout << req << std::endl;
}

void Inspect::inspect_response_activity(const std::string &msg, const Response &resp)
{
	write_curr_time();

	// ex.: Response has been fully sent; HTTP/1.1 200 OK (body size: 0)
	if (msg.empty())
	{
		std::cout << YEL "Response has been fully sent; ";
		std::cout << HTTP::stringProtocol(resp.protocol) << " ";
		std::cout << resp.status_code << " ";
		std::cout << HTTP::getReasonPhrase(resp.status_code) << " ";
		std::cout << "(body size: " << resp.body.size() << ")" << DEF << std::endl;
	}
	else
		std::cerr << YEL "Response has encountered an error; " << msg << DEF << std::endl;

	if (debug)
		std::cout << resp << std::endl;
}

void Inspect::inspect_cgi_activity(const std::string &msg, const int &fd)
{
	write_curr_time();

	// ex.: Cgi 5 has started execution;    
	// ex.: Cgi 5 has finished execution;
	// ex.: Cgi 5 has failed to execute due to: an error;
	std::cout << GRN "Cgi " << fd << " has " << msg << ";" DEF << std::endl;
}


void Inspect::inspect_removed_cl(const t_remove_reason &reason, const int &fd)
{
	write_curr_time();

	// ex.: Client 5 has been disconnected from the server; Reason: CGI execution time out    
	std::cout << RED "Client " << fd << " has been disconnected from the server; ";
	std::cout << "Reason: ";
	switch (reason)
	{
	case TIMEOUT:
		std::cout << "Client time out";
		break;

	case TIMEOUT_CGI:
		std::cout << "CGI execution time out";
		break;
	
	case RECV_FAIL:
		std::cout << "Failed to recieve a request from the Client";
		break;
	
	case WRITE_FAIL:
		std::cout << "Failed to write a response to the Client";
		break;
	
	case CLOSE_CONNECTION:
		std::cout << "Connection has been terminated";
		break;
		
	case SERVER_CLOSE:
		std::cout << "The Server has closed";
		break;

	default:
		std::cout << "Unknown...";
		break;
	}
	std::cout << DEF << std::endl;
}

namespace response_utils { std::string date_format(time_t timestamp); }

void Inspect::write_curr_time(void)
{
	std::cout << "[" << response_utils::date_format(time(NULL)) << "]	";
}
