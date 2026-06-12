#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>

#include "../../inc/serverConfig/ServerConfig.hpp"

// =====>┊( CONFIG )┊

/**
 * @class Config
 * @brief Owner and facade for the parsed server configuration tree.
 *
 * Responsibilities:
 * - Load a configuration file (lexer + parser).
 * - Store the normalized list of `ServerConfig` objects.
 * - Provide read-only accessors used by the runtime server.
 */
class Config
{
	public:
		Config(const std::string &filepath);
		~Config(void);

		const ServerConfig *getServer(const std::string &listen, const std::string &hostHeader) const;
		const std::vector<ServerConfig> &getallServers(void) const;

		std::vector<int> getUniquePorts(void) const;
		std::vector<ListenAddress> getUniqueListen(void) const;

	private:
		void load(const std::string &filePath);

		std::vector<ServerConfig>	_servers;


};

std::ostream &operator<<(std::ostream &out, Config const &src);
