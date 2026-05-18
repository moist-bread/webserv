#include "../../inc/serverConfig/Config.hpp"

#include "../../inc/ansi_color_codes.h"

#include <sstream>
#include <algorithm>
#include <map>

Config::Config(void) {}

Config::Config(Config const &src) { *this = src; }

Config::~Config(void) {}

Config &Config::operator=(Config const &src)
{
	if (this != &src)
		_servers = src._servers;
	return (*this);
}

//	Load the configuration file information into the config class
void Config::load(const std::string &filePath)
{
	//	tokenize the file
	std::vector<t_token> tokenFile = Lexer::tokenizeFile(filePath);
	std::cout << tokenFile << std::endl;

	//	Parse the tokens and dump the tokens into the Config class variables
	//	The parser class instantiate with tokens and then parse into a given servers vector 
	_servers = ConfigParser(tokenFile).parse();
}

const ServerConfig* Config::getServer(const std::string &listen, const std::string &hostHeader) const
{
	const ServerConfig *matchServer = NULL;
	for (size_t i = 0; i < this->_servers.size(); ++i)
	{
		if (this->_servers[i].getListenString() == listen)
		{
			if (this->_servers[i].isServerName(hostHeader))
				return (&this->_servers[i]);
			if (matchServer == NULL)
				matchServer = &this->_servers[i];
		}
	}
	return (matchServer);
}

const std::vector<ServerConfig>& Config::getallServers(void) const
{
	return (this->_servers);
}

std::vector<int> Config::getUniquePorts(void) const
{
	std::vector<int> uniquePorts;
	for (size_t i = 0; i < this->_servers.size(); ++i)
	{
		if (std::find(uniquePorts.begin(), uniquePorts.end(), this->_servers[i].getListenPort()) == uniquePorts.end())
			uniquePorts.push_back(this->_servers[i].getListenPort());
	}
	return (uniquePorts);
}

std::vector<ListenAddress> Config::getUniqueListen(void) const
{
	std::vector<ListenAddress> uniqueListen;
	std::vector<std::string> listenAdded;
	for (size_t i = 0; i < this->_servers.size(); ++i)
	{
		if (std::find(listenAdded.begin(), listenAdded.end(), this->_servers[i].getListenString()) == listenAdded.end())
		{
			listenAdded.push_back(this->_servers[i].getListenString());
			uniqueListen.push_back(this->_servers[i].getListenAddress());
		}
	}
	return (uniqueListen);
}

std::ostream &operator<<(std::ostream &out, Config const &src)
{
	out << "========================================\n";
	out << "         WEBSERV CONFIGURATION          \n";
	out << "========================================\n";
	
	const std::vector<ServerConfig>& servers = src.getallServers(); 
	
	for (size_t i = 0; i < servers.size(); ++i) {
		out << servers[i];
		out << "----------------------------------------\n";
	}
	return out;
}
