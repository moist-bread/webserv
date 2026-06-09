#pragma once

// ==┊ needed libs by class
#include "HTTP.hpp"

# ifndef DEBUG
#  define DEBUG 1
# endif

class Server;
class Request;
class Response;
// class CgiHandler;

enum t_remove_reason
{
	TIMEOUT,
	TIMEOUT_CGI,
	RECV_FAIL,
	WRITE_FAIL,
	CLOSE_CONNECTION,
	SERVER_CLOSE
};

// =====>┊( INSPECT )┊

class Inspect
{
public:
	static void inspect_server_activity(const std::string &msg, const Server &sv);
	static void inspect_client_activity(const std::string &msg, const int &fd, const int &port);
	static void inspect_request_activity(const std::string &msg, const Request &req);
	static void inspect_response_activity(const std::string &msg, const Response &resp);
	static void inspect_cgi_activity(const std::string &msg, const int &fd);
	static void inspect_removed_cl(const t_remove_reason &reason, const int &fd);
	static const bool debug;
	
private:
	Inspect(void) {};
	Inspect(Inspect const &src) { *this = src; };
	~Inspect(void) {};
	Inspect &operator=(Inspect const &src) { (void)src; return(*this); }; // can be removed if canonical form is unecessary


	static void write_curr_time(void);
};
