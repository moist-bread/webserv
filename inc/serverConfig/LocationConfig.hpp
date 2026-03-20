#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <map>
#include <vector>

#include "../requests/HTTP.hpp"
#include "../ansi_color_codes.h"

// =====>┊( CLASS )┊

class LocationConfig
{
	public:
		LocationConfig(void); 							// default constructor
		LocationConfig(LocationConfig const &source);	// copy constructor
		~LocationConfig(void);							// destructor

		LocationConfig &operator=(LocationConfig const &source); // copy assignment operator overload

		std::string				path;           // e.g., "/images"
		std::string				root;           // Directory to search files in
		std::string				index;          // Default file (e.g., "index.html")
		bool					autoindex;      // Directory listing (on/off)
		std::vector<t_method>	allowedMethods; // e.g., ["GET", "POST"]
		std::string				uploadStore;    // Where to save POST uploads
	
		// Redirections (HTTP 301/302)
		int						returnCode;     // e.g., 301
		std::string				returnUrl;      // e.g., "https://google.com"
	
		// CGI execution
		// Map extension to the executable (e.g., ".php" -> "/usr/bin/php-cgi")
		std::map<std::string, std::string>	cgi;            
};

std::ostream &operator<<(std::ostream &out, LocationConfig const &source);
