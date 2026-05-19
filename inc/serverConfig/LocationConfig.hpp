#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <map>
#include <vector>

#include "../requests/HTTP.hpp"
#include "../ansi_color_codes.h"

// =====>┊ LocationConfig ┊

/**
 * @brief Configuration for a single location (route) inside a server block.
 *
 * `LocationConfig` describes how the server should handle requests whose
 * request-target begins with `path`. It contains routing information such
 * as root directory, index files, allowed methods, upload location, redirects
 * and CGI mappings.
 */
struct LocationConfig
{
	LocationConfig(void); 							// default constructor
	LocationConfig(LocationConfig const &source);	// copy constructor
	LocationConfig &operator=(LocationConfig const &source); // copy assignment operator overload
	~LocationConfig(void);							// destructor

	//	Getters
	const std::string &getRoot(void) const;
	const std::vector<std::string> &getIndex(void) const;
	bool isAutoIndexOn(void) const;
	bool isMethodAllowed(t_method method) const;
	const std::string &getUploadStore(void) const;
	bool isRedirect(void) const;
	t_status_code getReturnCode(void) const;
	const std::string &getReturnUrl(void) const;
	std::string getCgiExecutable(const std::string &extension) const;

	//	Variables
	std::string					path;           // e.g., "/images"
	std::string					root;           // Directory to search files in
	std::vector<std::string>	index;          // Default file (e.g., "index.html")
	int							autoindex;      // Directory listing (on/off)
	std::vector<t_method>		allowedMethods; // e.g., ["GET", "POST"]
	std::string					uploadStore;    // Where to save POST uploads
	
	// Redirections
	t_status_code				returnCode;     // e.g., 301
	std::string					returnUrl;      // e.g., "https://google.com"
	
	// CGI execution
	std::map<std::string, std::string>	cgi;	// Map extension to the executable (e.g., ".php" -> "/usr/bin/php-cgi")
};

std::ostream &operator<<(std::ostream &out, LocationConfig const &source);
