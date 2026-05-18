#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>

#include "../../inc/serverConfig/Lexer.hpp"
#include "../../inc/serverConfig/ConfigParser.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"

// =====>┊( CONFIG )┊

class Config
{
	public:
		Config(void);
		Config(const Config &src);
		~Config(void);
		Config &operator=(Config const &src);

		void load(const std::string &filePath);

		const ServerConfig *getServer(const std::string &listen, const std::string &hostHeader) const;
		const std::vector<ServerConfig> &getallServers(void) const;

		std::vector<int> getUniquePorts(void) const;
		std::vector<ListenAddress> getUniqueListen(void) const;

	private:
		std::vector<ServerConfig>	_servers;

};

std::ostream &operator<<(std::ostream &out, Config const &src);

// class Config {
// private:
//     std::vector<ServerConfig> _servers;

// public:
    // What it does: Loops through all _servers, extracts their 'port' numbers, 
    // removes duplicates, and returns the list.
    // Why: You use this in main.cpp to know how many poll()/select() sockets to open!
    // std::vector<int> getUniquePorts(void) const;

    // What it does: 
    // 1. Finds all servers listening on the given 'port'.
    // 2. Checks if the 'hostHeader' (e.g., "example.com") matches any of their 'server_names'.
    // 3. Returns a pointer to the exact matching server (or the first/default server on that port if no name matches).
    // const ServerConfig* getServer(int port, const std::string &hostHeader) const;
// };