#include "../../inc/serverConfig/Parser.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include <sstream>
#include <stdexcept>

Parser::Parser(const std::vector<t_token> &tokens) : _cursor(0), _tokens(tokens)
{
	_serverHandlers["listen"] = &Parser::_serverListen;
	_serverHandlers["server_name"] = &Parser::_serverName;
	_serverHandlers["client_max_body_size"] = &Parser::_serverMaxBodySize;
	_serverHandlers["error_page"] = &Parser::_serverErrorPage;
	_serverHandlers["location"] = &Parser::_serverLocation;

	_locationHandlers["root"] = &Parser::_locationRoot;
	_locationHandlers["index"] = &Parser::_locationIndex;
	_locationHandlers["autoindex"] = &Parser::_locationAutoIndex;
	_locationHandlers["allow_methods"] = &Parser::_locationAllowMethods;
	_locationHandlers["return"] = &Parser::_locationReturn;
	_locationHandlers["cgi"] = &Parser::_locationCgi;
	_locationHandlers["upload_store"] = &Parser::_locationUploadStore;

	std::cout << GRN "the Parser ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

// !! Needed?
// Parser::Parser(void) : _tokens()
// {
// 	std::cout << GRN "the Parser ";
// 	std::cout << UCYN "has been created" DEF << std::endl;
// }

Parser::Parser(Parser const &source) : _cursor(0), _tokens(source._tokens)
{
	*this = source;
	std::cout << GRN "the Parser ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Parser::~Parser(void)
{
	std::cout << GRN "the Parser ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Parser &Parser::operator=(Parser const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
	{
		this->_cursor = source._cursor;
		this->_serverHandlers = source._serverHandlers;
		this->_locationHandlers = source._locationHandlers;
	}
	return (*this);
}

std::ostream &operator<<(std::ostream &out, Parser const &source)
{
	(void)source;
	out << BLU "Parser";
	out << DEF << std::endl;
	return (out);
}

void Parser::parse(std::vector<ServerConfig> &servers)
{
	while (_currentToken().type != TOKEN_EOF)
	{
		if (_currentToken().type == TOKEN_KEYWORD && _currentToken().content == "server")
		{
			_advanceToken();
			ServerConfig newServer = _parseServerBlock();
			servers.push_back(newServer);
			// std::cout << "SERVER_BLOCK -> port = " << servers[0].port << "\n";
		}
		else
			throw std::runtime_error("Syntax error at line " /* + line number */ ": Expected 'server' block");
	}
}

ServerConfig Parser::_parseServerBlock(void)
{
	ServerConfig newServer;
	_expect(TOKEN_LBRACE);
	while (_currentToken().type != TOKEN_RBRACE)
	{
		t_token directiveToken = _expect(TOKEN_KEYWORD);
		std::string directive = directiveToken.content;
		if (_serverHandlers.count(directive) > 0)
		{
			ServerHandler handler = _serverHandlers[directive];
			(this->*handler)(newServer);
		}
		else
		{
			std::stringstream ss;
			ss << "Syntax error at line " << directiveToken.line
				<< ": Unknown directive '" << directiveToken.content << "'";
			throw std::runtime_error(ss.str());
		}
	}
	_expect(TOKEN_RBRACE);
	return (newServer);
}

LocationConfig Parser::_parseLocationBlock(const std::string &path)
{
	LocationConfig newLocation;
	newLocation.path = path;
	_expect(TOKEN_LBRACE);
	while (_currentToken().type != TOKEN_RBRACE)
	{
		t_token directiveToken = _expect(TOKEN_KEYWORD);
		std::string directive = directiveToken.content;
		if (_locationHandlers.count(directive) > 0)
		{
			LocationHandler handler = _locationHandlers[directive];
			(this->*handler)(newLocation);
		}
		else
		{
			std::stringstream ss;
			ss << "Syntax error at line " << directiveToken.line
				<< ": Unknown locationdirective '" << directiveToken.content << "'";
			throw std::runtime_error(ss.str());
		}
	}
	_expect(TOKEN_RBRACE);
	if (newLocation.allowedMethods.empty())
		newLocation.allowedMethods.push_back(GET);
	return (newLocation);
}

//*	----- ServerHandler functions ----- */

void Parser::_serverListen(ServerConfig &server)
{
	std::string listen;
	_extractSingleKeyword(listen);
	size_t colonPos = listen.find(':');
	if (colonPos != std::string::npos)
	{
		server.host = listen.substr(0, colonPos);
		server.port = std::atoi(listen.substr(colonPos + 1).c_str());
	}
	else
	{
		server.host = "0.0.0.0";
		server.port = std::atoi(listen.c_str());
	}
	_expect(TOKEN_SEMICOLON);
}

void Parser::_serverName(ServerConfig &server)
{
	_extractKeywordVector(server.serverNames);
	_expect(TOKEN_SEMICOLON);
	if (server.serverNames.empty())
		throw std::runtime_error("Syntax error: server_name directive is empty");
}

void Parser::_serverMaxBodySize(ServerConfig &server)
{
	std::string sizeValue;
	_extractSingleKeyword(sizeValue);
	if (!sizeValue.empty() && sizeValue[sizeValue.size() - 1] == 'M')
		sizeValue.erase(sizeValue.size() - 1);
	server.clientMaxBodySize = std::strtoul(sizeValue.c_str(), NULL, 10);
	if (!sizeValue.empty() && sizeValue[sizeValue.size() - 1] == 'M')
		server.clientMaxBodySize *= 1048576;
	_expect(TOKEN_SEMICOLON);
}

void Parser::_serverErrorPage(ServerConfig &server)
{
	std::vector<std::string> tempArgs;
	_extractKeywordVector(tempArgs);
	_expect(TOKEN_SEMICOLON);
	if (tempArgs.size() < 2)
      		throw std::runtime_error("Syntax error: error_page needs at least one code and a URI");
	std::string uri = tempArgs.back();
	tempArgs.pop_back();
	for (size_t i = 0; i < tempArgs.size(); ++i)
	{
		int errorCode = std::atoi(tempArgs[i].c_str());
		server.errorPages[static_cast <t_status_code>(errorCode)] = uri;
	}
}

void Parser::_serverLocation(ServerConfig &server)
{
	t_token pathToken = _expect(TOKEN_KEYWORD);
	LocationConfig newLocation = _parseLocationBlock(pathToken.content);
	server.locations.push_back(newLocation);
}

//*	----- LocationHandler functions ----- */

void Parser::_locationRoot(LocationConfig &location)
{
	_extractSingleKeyword(location.root);
	_expect(TOKEN_SEMICOLON);
}

void Parser::_locationIndex(LocationConfig &location)
{
	_extractSingleKeyword(location.index);
	_expect(TOKEN_SEMICOLON);
}

void Parser::_locationAutoIndex(LocationConfig &location)
{
	std::string autoindex;
	_extractSingleKeyword(autoindex);
	if (autoindex != "on" && autoindex != "off")
		throw std::runtime_error("Syntax error: autoindex must be on/off");
	location.autoindex = autoindex == "on" ?  true : false;
	_expect(TOKEN_SEMICOLON);
}

void Parser::_locationAllowMethods(LocationConfig &location)
{
	while (_currentToken().type == TOKEN_KEYWORD)
	{
		t_method method = HTTP::getMethod(_currentToken().content);
		location.allowedMethods.push_back(method);
		_advanceToken();
	}
	_expect(TOKEN_SEMICOLON);
	if (location.allowedMethods.empty())
		throw std::runtime_error("Syntax error: allow_methods directive is empty");
}

void Parser::_locationReturn(LocationConfig &location)
{
	std::string returnCode;
	_extractSingleKeyword(returnCode);
	location.returnCode = std::atoi(returnCode.c_str());
	_extractSingleKeyword(location.returnUrl);
	_expect(TOKEN_SEMICOLON);
}

void Parser::_locationCgi(LocationConfig &location)
{
	std::string ext;
	std::string exec;
	_extractSingleKeyword(ext);
	_extractSingleKeyword(exec);
	location.cgi[ext] = exec;
	_expect(TOKEN_SEMICOLON);
}

void Parser::_locationUploadStore(LocationConfig &location)
{
	_extractSingleKeyword(location.uploadStore);
	_expect(TOKEN_SEMICOLON);
}

//*	----- Helpers ----- */

t_token Parser::_expect(e_token_type expected_type)
{
	if (this->_cursor >= _tokens.size())
		throw std::runtime_error("Syntax error: Unexpected end of file. Missing '}' or ';'");
	t_token current = _currentToken();
	if (current.type == expected_type)
	{
		_advanceToken();
		return (current);
	}
	std::stringstream ss;
    ss << "Syntax error at line " << _currentToken().line << ": Unexpected token '" 
		<< _currentToken().content<< "'";
    throw std::runtime_error(ss.str());
}

const t_token &Parser::_currentToken(void) const
{
	if (this->_cursor >= this->_tokens.size())
		throw std::runtime_error("Parser error: Unexpected end of tokens");
	return (this->_tokens[this->_cursor]);
}

void Parser::_advanceToken(void)
{
	if (this->_cursor < this->_tokens.size())
		this->_cursor++;
}

void Parser::_extractSingleKeyword(std::string &destination)
{
	t_token token = _expect(TOKEN_KEYWORD);
	destination = token.content;
}

void Parser::_extractKeywordVector(std::vector<std::string> &destination)
{
	while (_currentToken().type == TOKEN_KEYWORD)
	{
		destination.push_back(_currentToken().content);
		_advanceToken();
	}
}
