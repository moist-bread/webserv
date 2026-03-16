#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <map>
#include <vector>

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

		int							port;               // e.g., 8080
		std::string					host;               // e.g., "127.0.0.1"
		std::vector<std::string>	serverNames;        // e.g., ["example.com", "www.example.com"]
		size_t						clientMaxBodySize;  // Limit for uploads (e.g., 1048576 for 1MB)

		// Default error pages
		// Maps the HTTP error code to a file (e.g., 404 -> "/errors/404.html")
		std::map<int, std::string>	errorPages;         

		// The routes that belong to this server
		std::vector<LocationConfig>	locations;          
};

std::ostream &operator<<(std::ostream &out, ServerConfig const &source);
