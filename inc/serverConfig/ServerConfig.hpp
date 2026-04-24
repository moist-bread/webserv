#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <map>
#include <vector>
#include "../requests/HTTP.hpp"

#include "../ansi_color_codes.h"
#include "LocationConfig.hpp"

// =====>┊( CLASS )┊

class ServerConfig
{
	public:
		ServerConfig(void); 				// default constructor
		ServerConfig(ServerConfig const &source);	// copy constructor
		~ServerConfig(void);				// destructor

		ServerConfig &operator=(ServerConfig const &source); // copy assignment operator overload

		std::string					host;               // e.g., "127.0.0.1"
		int							port;               // e.g., 8080
		std::string					listenAddr;			// host:port combined into a string
		std::vector<std::string>	serverNames;        // e.g., ["example.com", "www.example.com"]
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
