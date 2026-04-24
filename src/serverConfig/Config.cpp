#include "../../inc/serverConfig/Config.hpp"
#include "../../inc/serverConfig/Lexer.hpp"
#include "../../inc/serverConfig/Parser.hpp"
#include <sstream>
#include <algorithm>

Config::Config(void)
{
	std::cout << GRN "the Config ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Config::Config(Config const &source)
{
	*this = source;
	std::cout << GRN "the Config ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Config::~Config(void)
{
	std::cout << GRN "the Config ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Config &Config::operator=(Config const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

//	Load the configuration file information into the config class
void Config::load(const std::string &filePath)
{
	//	tokenize the file
	std::vector<t_token> fileTokens = Lexer::tokenizeFile(filePath);
	std::cout << fileTokens << std::endl;

	//	Parse the tokens and dump the tokens into the Config class variables
	//	The parser class instantiate with tokens and then parse into a given servers vector 
	Parser configFile(fileTokens);
	configFile.parse(this->_servers);

	this->_validateServers();
}

const std::vector<ServerConfig>& Config::getServers(void) const
{
	return (this->_servers);
}

void Config::_validateServers(void) const
{
	std::map<std::string, std::vector<std::string> > serverClaimedNames;
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::vector<std::string> &claimedNames = serverClaimedNames[_servers[i].listenAddr];
		_validate_HostPort(_servers[i]);
		_validate_ServerNames(_servers[i]);
		_validate_BodySize(_servers[i]);
		_validate_ErrorPages(_servers[i]);
		_validate_NameServer_Collision(_servers[i], claimedNames);
		_add_to_ClaimedNames(_servers[i], claimedNames);
	}
}

void Config::_validate_HostPort(const ServerConfig &server)
{
	if (server.port < 1 || server.port > 65535)
		throw std::runtime_error("Config error: IP"); // ! Write error
	if (std::count(server.host.begin(), server.host.end(), '.') != 3)
		throw std::runtime_error("Config error: IP"); // ! Write error
	if (server.host == "localhost" || server.host == "0.0.0.0")
		return ;

	std::string octet;
	std::stringstream ss(server.host);
	while (std::getline(ss, octet, '.'))
	{
		if (octet.empty())
			throw std::runtime_error("Config error: IP"); // ! Write error
		for (size_t i = 0; i < octet.length(); i++)
		{
			if (!std::isdigit(octet[i]))
				throw std::runtime_error("Config error: IP"); // ! Write error
		}
		int octet_value = std::atoi(octet.c_str());
		if (octet_value < 0 || octet_value > 255)
			throw std::runtime_error("Config error: IP"); // ! Write error
	}	
}

void Config::_validate_ServerNames(const ServerConfig &server)
{
	for (size_t i = 0; i < server.serverNames.size(); ++i)
	{
		const std::string &currentName = server.serverNames[i];
		if (currentName == "_")
			continue ;
		if (currentName[0] == '.' || currentName[0] == '-' || 
				currentName[currentName.length() - 1] == '.' ||
				currentName[currentName.length() - 1] == '-')
			throw std::runtime_error("Config error: Server Names"); // ! Write error
		for (size_t j = 0; j < currentName.length(); ++j)
		{
			char c = currentName[j];
			if (!std::isalnum(c) && c != '.' && c != '-')
				throw std::runtime_error("Config error: Server Names"); // ! Write error
			if (c == '.' && currentName[j - 1] == '.')
				throw std::runtime_error("Config error: Server Names"); // ! Write error
		}
	}
}

void Config::_validate_BodySize(const ServerConfig &server)
{
	if (server.clientMaxBodySize < 1 || server.clientMaxBodySize > ServerConfig::MAX_CLIENT_MAX_BODY_SIZE)
		throw std::runtime_error("Config error: Client Max Body Size"); // ! Write error
}

void Config::_validate_ErrorPages(const ServerConfig &server)
{
	std::map<t_status_code, std::string>::const_iterator it;
	for (it = server.errorPages.begin(); it != server.errorPages.end(); ++it)
	{
		int code = it->first;
		if (code < 400 || code > 599)
			throw std::runtime_error("Config error: Client Max Body Size"); // ! Write error
	}
}

void Config::_validate_NameServer_Collision(const ServerConfig &server, const std::vector<std::string> &claimedNames)
{
	for (size_t i = 0; i < claimedNames.size(); i++)
	{
		const std::string &name = claimedNames[i];
		for (size_t j = 0; j < server.serverNames.size(); ++j)
		{
			if (name == server.serverNames[j])
				throw std::runtime_error("Config error: Name Server Collision"); // ! Write error
		}
	}
}

void Config::_add_to_ClaimedNames(const ServerConfig &server, std::vector<std::string> &dest)
{
	dest.insert(dest.end(), server.serverNames.begin(), server.serverNames.end());
}


std::ostream &operator<<(std::ostream &out, Config const &source)
{
	out << "========================================\n";
	out << "         WEBSERV CONFIGURATION          \n";
	out << "========================================\n";
	
	const std::vector<ServerConfig>& servers = source.getServers(); 
	
	for (size_t i = 0; i < servers.size(); ++i) {
		out << servers[i];
		out << "----------------------------------------\n";
	}
	return out;
}
