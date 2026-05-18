#include "../../inc/serverConfig/ServerConfig.hpp"

const int ServerConfig::DEFAULT_CLIENT_MAX_BODY_SIZE = 1048576;
const unsigned long ServerConfig::MAX_CLIENT_MAX_BODY_SIZE = 524288000;

ServerConfig::ServerConfig(void) : clientMaxBodySize(0) { listen.port = -1; }

ServerConfig::ServerConfig(ServerConfig const &source) { *this = source; }

ServerConfig::~ServerConfig(void) {}

ServerConfig &ServerConfig::operator=(ServerConfig const &source)
{
	if (this != &source)
	{
		this->listen.host = source.listen.host;
		this->listen.port = source.listen.port;
		this->listen.string = source.listen.string;
		this->serverNames = source.serverNames;
		this->root = source.root;
		this->clientMaxBodySize = source.clientMaxBodySize;
		this->errorPages = source.errorPages;
		this->locations = source.locations;
	}
	return (*this);
}

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

const std::string &ServerConfig::getListenHost(void) const { return (this->listen.host); }

int ServerConfig::getListenPort(void) const { return (this->listen.port); }

const std::string &ServerConfig::getListenString(void) const { return (this->listen.string); }

const ListenAddress &ServerConfig::getListenAddress(void) const { return (this->listen); }

const std::vector<std::string> &ServerConfig::getServerNames(void) const { return (this->serverNames); }

bool ServerConfig::isServerName(const std::string &serverName) const
{
	for (size_t i = 0; i < this->serverNames.size(); ++i)
	{
		if (this->serverNames[i] == serverName)
			return (true);
	}
	return (false);
}

const std::string &ServerConfig::getRoot(void) const { return (this->root); }

size_t ServerConfig::getClientMaxBodySize(void) const { return (this->clientMaxBodySize); }

std::string ServerConfig::getErrorPage(t_status_code code) const
{
	std::map<t_status_code, std::string>::const_iterator it = this->errorPages.find(code);
	if (it != this->errorPages.end())
		return (it->second);
	return ("");
}

const LocationConfig* ServerConfig::matchLocation(const std::string& uri) const
{
	const LocationConfig *LocationMatched = NULL;
	size_t longestMatchPrefix = 0;
	for (size_t i = 0; i < this->locations.size(); ++i)
	{
		const std::string &pathLocation = this->locations[i].path;
		if (uri.find(pathLocation) == 0)
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

std::ostream &operator<<(std::ostream &out, ServerConfig const &source)
{
	out << "  [Server Block]\n";
	out << "    Host: " << source.getListenHost() << "\n";
	out << "    Port: " << source.getListenPort() << "\n";
	out << "    Listen adress: " << source.getListenString() << "\n";
	out << "    Root: " << source.getRoot() << "\n";
	out << "    Client Max Body Size: " << source.getClientMaxBodySize() << "\n";

	out << "    Server Names: ";
	for (size_t i = 0; i < source.serverNames.size(); ++i) {
		out << source.serverNames[i] << " ";
	}
	out << "\n";

	out << "    Error Pages:\n";
	for (std::map<t_status_code, std::string>::const_iterator it = source.errorPages.begin(); it != source.errorPages.end(); ++it) {
		out << "      " << it->first << " -> " << it->second << "\n";
	}

	out << "    Locations:\n";
	for (size_t i = 0; i < source.locations.size(); ++i) {
		out << source.locations[i]; 
		out << "\n";
	}
	return out;
}
