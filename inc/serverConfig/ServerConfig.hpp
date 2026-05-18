#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <map>
#include <vector>
#include "../requests/HTTP.hpp"

#include "../ansi_color_codes.h"
#include "LocationConfig.hpp"

// =====>┊( CLASS )┊

struct ListenAddress
{
	std::string	host;		// e.g., "127.0.0.1"
	int			port;		// e.g., 8080
	std::string	string;		// host:port combined into a string

	ListenAddress() : port(-1) {};
};

struct ServerConfig
{
	ServerConfig(void); 				// default constructor
	ServerConfig(ServerConfig const &source);	// copy constructor
	ServerConfig &operator=(ServerConfig const &source); // copy assignment operator overload
	~ServerConfig(void);				// destructor

	const std::string &getListenHost(void) const;
	int getListenPort(void) const;
	const std::string &getListenString(void) const;
	const ListenAddress &getListenAddress(void) const;
	const std::vector<std::string> &getServerNames(void) const;
	bool isServerName(const std::string &serverName) const;
	std::string getServerUrl(void) const;
	const std::string &getRoot(void) const;
	size_t getClientMaxBodySize(void) const;
	std::string getErrorPage(t_status_code code) const;

	const LocationConfig* matchLocation(const std::string& uri) const;

	ListenAddress				listen;
	std::vector<std::string>	serverNames;        // e.g., ["example.com", "www.example.com"]
	std::string					root;
	size_t						clientMaxBodySize;  // Limit for uploads (e.g., 1048576 for 1MB)

	// Default error pages
	// Maps the HTTP error code to a file (e.g., 404 -> "/errors/404.html")
	std::map<t_status_code, std::string>	errorPages;	// 400 - 599 

	// The routes that belong to this server
	std::vector<LocationConfig>	locations;
	
	static const int DEFAULT_CLIENT_MAX_BODY_SIZE;
	static const unsigned long MAX_CLIENT_MAX_BODY_SIZE;
};

std::ostream &operator<<(std::ostream &out, ServerConfig const &source);

/*
struct ServerConfig {

    What it does: Compares the request URI against all location paths.
    CRITICAL LOGIC: It must find the "Longest Prefix Match". 
    If the user requests "/images/cats/cute.jpg", and you have location "/", 
    and location "/images", it MUST return the pointer to "/images".
    const LocationConfig* matchLocation(const std::string &uri) const;

    What it does: Returns clientMaxBodySize.
    size_t getClientMaxBodySize(void) const;

    What it does: Checks the errorPages map. If the map has a custom HTML 
    page for the requested error code (e.g., 404), return the string path. 
    If not, return an empty string "" (so the server knows to use a default).
    std::string getErrorPage(t_status_code code) const;
};
*/
