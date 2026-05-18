#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <map>
#include <vector>

#include "../requests/HTTP.hpp"
#include "../ansi_color_codes.h"

// =====>┊ LocationConfig ┊

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

/*
struct LocationConfig {

    --- Core Routing & Pathing ---
    What it does: Returns the physical root folder path.
    const std::string& getRoot(void) const;

    What it does: Returns the vector of index files (e.g., ["index.html", "index.php"]).
    const std::vector<std::string>& getIndex(void) const;

    What it does: Returns true if autoindex is on, false if off.
    bool isAutoindexOn(void) const;


    --- Security & Methods ---
    What it does: Loops through 'allowedMethods' vector. If 'method' is found, 
    return true. If not, return false. (Triggers a 405 error if false).
    bool isMethodAllowed(t_method method) const;


    --- Redirection (Return) ---
    What it does: Returns true if returnCode != NO_STATUS.
    bool isRedirect(void) const;

    What it does: Returns the HTTP code (e.g., 301, 302).
    t_status_code getRedirectCode(void) const;

    What it does: Returns the URL to redirect the user to.
    const std::string& getRedirectUrl(void) const;


    --- CGI (Script Execution) ---
    What it does: Checks the CGI map using the file extension (e.g., ".py"). 
    If found, returns the path to the executable (e.g., "/usr/bin/python3"). 
    If not found, returns an empty string.
    std::string getCgiExecutable(const std::string &extension) const;


    --- Uploads ---
    What it does: Returns the upload_store path. If empty, the location 
    does not accept file uploads.
    const std::string& getUploadStore(void) const;
};
*/