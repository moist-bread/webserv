#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>

class ServerConfig;

// =====>┊( CONFIG )┊

class Config
{
	public:
		Config(void);
		Config(const Config &src);
		~Config(void);
		Config &operator=(Config const &src);

		void load(const std::string &filePath);
		const std::vector<ServerConfig> &getServers(void) const;

	private:
		std::vector<ServerConfig>	_servers;

};

std::ostream &operator<<(std::ostream &out, Config const &src);
