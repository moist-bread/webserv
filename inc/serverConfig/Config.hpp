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

		void _validateServers(void) const;
		static void _validate_HostPort(const ServerConfig &server);
		static void _validate_ServerNames(const ServerConfig &server);
		static void _validate_BodySize(const ServerConfig &server);
		static void _validate_ErrorPages(const ServerConfig &server);
		static void _validate_NameServer_Collision(const ServerConfig &server, const std::vector<std::string> &claimedNames);
		static void _add_to_ClaimedNames(const ServerConfig &server, std::vector<std::string> &dest);
};

std::ostream &operator<<(std::ostream &out, Config const &src);
