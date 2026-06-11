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
		this->serverName = source.serverName;
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

	if (!this->serverName.empty())
	    domainOrIp = this->serverName;
	else if (this->listen.host == "127.0.0.1")
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
 * @brief Get the configured server name for this virtual server.
 * @return Reference to the server name string.
 */
const std::string &ServerConfig::getServerName(void) const { return (this->serverName); }

/**
 * @brief Check whether `serverName` is present in this server's `server_name` list.
 * @param serverName Name to check.
 * @return true if present, false otherwise.
 */
bool ServerConfig::isServerName(const std::string &serverName) const
{
	if (this->serverName == serverName)
		return (true);
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

/**
 * @brief Get the server-level default CGI mappings.
 * 
 * Returns the map of file extensions to CGI executable paths that apply
 * to all locations in this server (unless overridden at location level).
 * 
 * @return Reference to the CGI extension->executable map.
 */
const std::map<std::string, std::string> &ServerConfig::getCgi(void) const { return (this->cgi_default); }


static bool matchExtension(const std::string &uri, const std::string &ext)
{
	size_t extension = uri.find(ext);
	if (extension == std::string::npos || extension != uri.length() - ext.length())
		return (false);
	return (true);
}

/**
 * @brief Find the best matching `LocationConfig` for a request URI and method.
 *
 * The function handles two types of location matching:
 * 
 * 1. **CGI Pass Locations** (isCgiPass() == true): Matches by file extension.
 *    Extracts the extension from the location path and checks if the URI ends
 *    with that extension. If found and the method is allowed, returns immediately.
 * 
 * 2. **Normal Locations**: Matches by URI prefix. Returns exact match immediately.
 *    Otherwise tracks the location with the longest matching prefix.
 *
 * @param uri Request URI to match (e.g., "/images/logo.php").
 * @param method HTTP method being used, checked against allowed methods in CGI pass locations.
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
	out << "    Server Name: " << source.serverName << "\n";
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
