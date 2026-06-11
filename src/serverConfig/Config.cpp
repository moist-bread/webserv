#include "../../inc/serverConfig/Config.hpp"
#include "../../inc/serverConfig/Lexer.hpp" // Lexer::tokenizeFile
#include "../../inc/serverConfig/ConfigParser.hpp"

#include <algorithm> // std::find

/**
 * @brief Constructor.
 * 
 * Loads the configuration from the specified file upon initialization.
 * 
 * @param filePath Path to configuration file to load.
 */
Config::Config(const std::string &filePath) { this->load(filePath); }

/**
 * @brief Destructor.
 */
Config::~Config(void) {}

/**
 * @brief Load configuration from file.
 *
 * Tokenizes `filePath` with the `Lexer`, parses tokens with `ConfigParser`,
 * and stores the resulting vector of `ServerConfig` inside this object.
 *
 * @param filePath Path to configuration file to load.
 */
void Config::load(const std::string &filePath)
{
	std::vector<t_token> tokenFile = Lexer::tokenizeFile(filePath);
	_servers = ConfigParser(tokenFile).parse();
}

/**
 * @brief Find the best matching server for the given listen string and Host header.
 *
 * The function returns the `ServerConfig` whose listen string matches `listen` and
 * whose `server_name` matches `hostHeader` when present. If no name matches, it
 * returns the first server that listens on `listen` (the default for that socket),
 * or nullptr if none match.
 *
 * @param listen Host:port string to match (e.g. "0.0.0.0:80").
 * @param hostHeader HTTP Host header used for name-based virtual hosting.
 * @return Pointer to matching `ServerConfig`, or nullptr if not found.
 */
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

/**
 * @brief Return all parsed servers.
 * @return Const reference to the internal `std::vector<ServerConfig>`.
 */
const std::vector<ServerConfig>& Config::getallServers(void) const
{
	return (this->_servers);
}

/**
 * @brief Collect unique listen ports across all servers.
 * @return Vector of unique port numbers.
 */
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

/**
 * @brief Collect unique listen addresses (host+port) across all servers.
 *
 * The `ListenAddress` objects are copies suitable for socket setup.
 *
 * @return Vector of unique `ListenAddress` entries.
 */
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

/**
 * @brief Pretty-print the parsed configuration for debugging.
 * @param out Output stream to write to.
 * @param src Config instance to print.
 * @return Reference to the output stream.
 */
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
