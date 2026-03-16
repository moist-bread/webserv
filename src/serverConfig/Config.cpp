#include "../../inc/serverConfig/Config.hpp"
#include "../../inc/serverConfig/Lexer.hpp"
#include "../../inc/serverConfig/Parser.hpp"

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

	//	Parse the tokens and dump the tokens into the Config class variables
	//	The parser class instantiate with tokens and then parse into a given servers vector 
	Parser configFile(fileTokens);
	configFile.parse(this->_servers);
}

const std::vector<ServerConfig>& Config::getServers(void) const
{
	return (this->_servers);
}

std::ostream &operator<<(std::ostream &out, Config const &source)
{
	out << "========================================\n";
	out << "         WEBSERV CONFIGURATION          \n";
	out << "========================================\n";
	
	// Assuming you followed our earlier advice and made a getter!
	const std::vector<ServerConfig>& servers = source.getServers(); 
	
	for (size_t i = 0; i < servers.size(); ++i) {
		out << servers[i]; // Magically calls the ServerConfig print function!
		out << "----------------------------------------\n";
	}
	return out;
}
