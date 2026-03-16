#include "../../inc/serverConfig/LocationConfig.hpp"

LocationConfig::LocationConfig(void)
{
	std::cout << GRN "the LocationConfig ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

LocationConfig::LocationConfig(LocationConfig const &source)
{
	*this = source;
	std::cout << GRN "the LocationConfig ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

LocationConfig::~LocationConfig(void)
{
	std::cout << GRN "the LocationConfig ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

LocationConfig &LocationConfig::operator=(LocationConfig const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, LocationConfig const &source)
{
	out << "    [Location] Path: " << source.path << "\n";
	out << "      Root: " << source.root << "\n";
	out << "      Index: " << source.index << "\n";
	out << "      Autoindex: " << (source.autoindex ? "on" : "off") << "\n";
	out << "      Upload Store: " << source.uploadStore << "\n";
	
	if (source.returnCode != 0)
		out << "      Return: " << source.returnCode << " -> " << source.returnUrl << "\n";

	out << "      Allowed Methods: ";
	for (size_t i = 0; i < source.allowedMethods.size(); ++i) {
		out << source.allowedMethods[i] << " ";
	}
	out << "\n";

	out << "      CGI:\n";
	for (std::map<std::string, std::string>::const_iterator it = source.cgi.begin(); it != source.cgi.end(); ++it) {
		out << "        " << it->first << " -> " << it->second << "\n";
	}
	return out;
}
