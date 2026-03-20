#include "../../inc/serverConfig/ServerConfig.hpp"

ServerConfig::ServerConfig(void)
{
	std::cout << GRN "the ServerConfig ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

ServerConfig::ServerConfig(ServerConfig const &source)
{
	*this = source;
	std::cout << GRN "the ServerConfig ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

ServerConfig::~ServerConfig(void)
{
	std::cout << GRN "the ServerConfig ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

ServerConfig &ServerConfig::operator=(ServerConfig const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
	{
		this->port = source.port;
		this->host = source.host;
		this->serverNames = source.serverNames;
		this->clientMaxBodySize = source.clientMaxBodySize;
		this->errorPages = source.errorPages;
		this->locations = source.locations;
	}
	return (*this);
}

std::ostream &operator<<(std::ostream &out, ServerConfig const &source)
{
	out << "  [Server Block]\n";
	out << "    Host: " << source.host << "\n";
	out << "    Port: " << source.port << "\n";
	out << "    Client Max Body Size: " << source.clientMaxBodySize << "\n";

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
