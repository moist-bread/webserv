#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>

#include "../ansi_color_codes.h"
#include "ServerConfig.hpp"

// =====>┊( CLASS )┊

class Config
{
	public:
		Config(void); 					// default constructor
		Config(const Config &source);	// copy constructor
		~Config(void);					// destructor

		Config &operator=(Config const &source); // copy assignment operator overload

		void load(const std::string &filePath);
		const std::vector<ServerConfig> &getServers(void) const;

	private:
		std::vector<ServerConfig>	_servers;
		
};

std::ostream &operator<<(std::ostream &out, Config const &source);
