#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <map>
#include <vector>
#include "../http/HTTP.hpp" // t_status_code, t_method

#include "LocationConfig.hpp" // used: LocationConfig

// =====>┊( CLASS )┊

struct ListenAddress
{
	std::string host;	// e.g., "127.0.0.1"
	int port;			// e.g., 8080
	std::string string; // host:port combined into a string

	ListenAddress() : port(-1) {};
};

/**
 * @brief Plain-data container representing a virtual server block.
 *
 * `ServerConfig` holds all parsed configuration for a single virtual server
 * (listen address, server names, root, error pages and location routes).
 * It is designed as a lightweight POD-like struct: validation and parsing
 * responsibilities belong to the parser, while the runtime reads these
 * fields for socket setup and request routing.
 */
struct ServerConfig
{
	ServerConfig(void);									 // default constructor
	ServerConfig(ServerConfig const &source);			 // copy constructor
	ServerConfig &operator=(ServerConfig const &source); // copy assignment operator overload
	~ServerConfig(void);								 // destructor

	const std::string &getListenHost(void) const;
	int getListenPort(void) const;
	const std::string &getListenString(void) const;
	const ListenAddress &getListenAddress(void) const;
	const std::string &getServerName(void) const;
	bool isServerName(const std::string &serverName) const;
	std::string getServerUrl(void) const;
	const std::string &getRoot(void) const;
	size_t getClientMaxBodySize(void) const;
	std::string getErrorPage(t_status_code code) const;
	const std::map<std::string, std::string> &getCgi(void) const;

	const LocationConfig *matchLocation(const std::string &uri, const t_method &method) const;

	ListenAddress listen;
	std::string serverName; // e.g., ["example.com", "www.example.com"]
	std::string root_default;
	size_t clientMaxBodySize; // Limit for uploads (e.g., 1048576 for 1MB)

	std::map<t_status_code, std::string> errorPages; // 400 - 599 // Maps the HTTP error code to a file (e.g., 404 -> "/errors/404.html")

	std::map<std::string, std::string> cgi_default; // Map extension to the executable (e.g., ".php" -> "/usr/bin/php-cgi")

	// The routes that belong to this server
	std::vector<LocationConfig> locations;

	static const int DEFAULT_CLIENT_MAX_BODY_SIZE;
	static const unsigned long MAX_CLIENT_MAX_BODY_SIZE;
};

std::ostream &operator<<(std::ostream &out, ServerConfig const &source);
