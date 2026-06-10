#include "../../inc/serverConfig/ServerConfig.hpp"

#include <sstream> // std::ostringstream


const int ServerConfig::DEFAULT_CLIENT_MAX_BODY_SIZE = 1048576;
const unsigned long ServerConfig::MAX_CLIENT_MAX_BODY_SIZE = 524288000;

/**
 * @brief Construct a ServerConfig with default values.
 *
 * Sets `clientMaxBodySize` to 0 and `listen.port` to -1 to indicate
 * an uninitialized listen address.
 */
ServerConfig::ServerConfig(void) : clientMaxBodySize(0) { listen.port = -1; }

/**
 * @brief Copy constructor — perform member-wise copy.
 * @param source Source ServerConfig to copy.
 */
ServerConfig::ServerConfig(ServerConfig const &source) { *this = source; }

/**
 * @brief Destructor (trivial).
 */
ServerConfig::~ServerConfig(void) {}

/**
 * @brief Copy assignment operator.
 * @param source Source ServerConfig to assign from.
 * @return Reference to this instance.
 */
ServerConfig &ServerConfig::operator=(ServerConfig const &source)
{
	if (this != &source)
	{
		this->listen.host = source.listen.host;
		this->listen.port = source.listen.port;
		this->listen.string = source.listen.string;
		this->serverNames = source.serverNames;
		this->root_default = source.root_default;
		this->clientMaxBodySize = source.clientMaxBodySize;
		this->errorPages = source.errorPages;
		this->cgi_default = source.cgi_default;
		this->locations = source.locations;
	}
	return (*this);
}

/**
 * @brief Build a simple HTTP URL for this server using the first server name
 *        or the listen host as fallback.
 *
 * Example: "http://example.com:8080"
 *
 * @return A std::string containing the URL.
 */
std::string ServerConfig::getServerUrl(void) const 
{
	std::string baseUrl = "http://";
	std::string domainOrIp;

	if (!this->serverNames.empty())
	    domainOrIp = this->serverNames[0];
	else if (this->listen.host == "0.0.0.0")
	    domainOrIp = "localhost";
	else
	    domainOrIp = this->listen.host;

	std::ostringstream portStream;
	portStream << this->listen.port;

	return (baseUrl + domainOrIp + ":" + portStream.str());
}

/**
 * @brief Get the configured listen host string.
 * @return Reference to the listen host string.
 */
const std::string &ServerConfig::getListenHost(void) const { return (this->listen.host); }

/**
 * @brief Get the configured listen port number.
 * @return Listen port as int.
 */
int ServerConfig::getListenPort(void) const { return (this->listen.port); }

/**
 * @brief Get the combined listen host:port string.
 * @return Reference to the combined listen string.
 */
const std::string &ServerConfig::getListenString(void) const { return (this->listen.string); }

/**
 * @brief Get the `ListenAddress` struct for this server.
 * @return Reference to the `ListenAddress` member.
 */
const ListenAddress &ServerConfig::getListenAddress(void) const { return (this->listen); }

/**
 * @brief Get the configured `server_name` entries.
 * @return Reference to vector of server names.
 */
const std::vector<std::string> &ServerConfig::getServerNames(void) const { return (this->serverNames); }

/**
 * @brief Check whether `serverName` is present in this server's `server_name` list.
 * @param serverName Name to check.
 * @return true if present, false otherwise.
 */
bool ServerConfig::isServerName(const std::string &serverName) const
{
	for (size_t i = 0; i < this->serverNames.size(); ++i)
	{
		if (this->serverNames[i] == serverName)
			return (true);
	}
	return (false);
}

/**
 * @brief Get the configured document root for this server.
 * @return Reference to the root path string.
 */
const std::string &ServerConfig::getRoot(void) const { return (this->root_default); }

/**
 * @brief Get the client max body size limit in bytes.
 * @return Size limit in bytes.
 */
size_t ServerConfig::getClientMaxBodySize(void) const { return (this->clientMaxBodySize); }

/**
 * @brief Retrieve the error page path for a given status code.
 * @param code HTTP status code to lookup.
 * @return File path to error page or empty string when not configured.
 */
std::string ServerConfig::getErrorPage(t_status_code code) const
{
	std::map<t_status_code, std::string>::const_iterator it = this->errorPages.find(code);
	if (it != this->errorPages.end())
		return (it->second);
	return ("");
}

const std::map<std::string, std::string> &ServerConfig::getCgi(void) const { return (this->cgi_default); }


static bool matchExtension(const std::string &uri, const std::string &ext)
{
	size_t extension = uri.find(ext);
	if (extension == std::string::npos || extension != uri.length() - ext.length())
		return (false);
	return (true);
}

/**
 * @brief Find the best matching `LocationConfig` for a request URI.
 *
 * The function performs prefix matching against each location's `path` and
 * returns the location with the longest matching prefix. An exact match is
 * returned immediately.
 *
 * @param uri Request URI to match (e.g., "/images/logo.png").
 * @return Pointer to the matched `LocationConfig`, or nullptr if none match.
 */
const LocationConfig* ServerConfig::matchLocation(const std::string& uri, const t_method &method) const
{
	const LocationConfig *LocationMatched = NULL;
	size_t longestMatchPrefix = 0;
	for (size_t i = 0; i < this->locations.size(); ++i)
	{
		const std::string &pathLocation = this->locations[i].getPath();
		if (this->locations[i].isCgiPass())
		{
			std::string extension = pathLocation.substr(1);
			if (matchExtension(uri, extension) && this->locations[i].isMethodAllowed(method))
				return (&this->locations[i]);
		}
		else if (uri.find(pathLocation) == 0)
		{
			if (pathLocation == uri)
				return (&this->locations[i]);
			if (pathLocation.length() > longestMatchPrefix)
			{
				longestMatchPrefix = pathLocation.length();
				LocationMatched = &this->locations[i];
			}
		}
	}
	return (LocationMatched);
}


/**
 * @brief Pretty-print a `ServerConfig` for debugging.
 * @param out Output stream.
 * @param source ServerConfig to print.
 * @return Reference to the output stream.
 */
std::ostream &operator<<(std::ostream &out, ServerConfig const &source)
{
	out << "  [Server Block]\n";
	out << "    Host: " << source.getListenHost() << "\n";
	out << "    Port: " << source.getListenPort() << "\n";
	out << "    Listen adress: " << source.getListenString() << "\n";

	out << "    Server Names: ";
	for (size_t i = 0; i < source.serverNames.size(); ++i) {
		out << source.serverNames[i] << " ";
	}
	out << "\n";

	out << "    Client Max Body Size: " << source.getClientMaxBodySize() << "\n";

	out << "    Error Pages:\n";
	for (std::map<t_status_code, std::string>::const_iterator it = source.errorPages.begin(); it != source.errorPages.end(); ++it) {
		out << "      " << it->first << " -> " << it->second << "\n";
	}
	
	out << "    Default Root: " << source.getRoot() << "\n";
	out << "    Default CGI:\n";
	for (std::map<std::string, std::string>::const_iterator it = source.cgi_default.begin(); it != source.cgi_default.end(); ++it)
	{
		out << "        " << it->first << " -> " << it->second << "\n";
	}
	out << "\n";

	out << "    Locations:\n";
	for (size_t i = 0; i < source.locations.size(); ++i) {
		out << source.locations[i]; 
		out << "\n";
	}
	return out;
}
