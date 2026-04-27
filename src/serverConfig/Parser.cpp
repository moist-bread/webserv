#include "../../inc/serverConfig/Parser.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include <sstream>
#include <stdexcept>

/**
 * @brief Construct parser and initialize directive dispatch tables.
 * @param tokens Immutable stream of lexer tokens.
 */
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

/**
 * @brief Destroy parser instance.
 */
Parser::~Parser(void)
{
	std::cout << GRN "the Parser ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

/**
 * @brief Parse all top-level server blocks until EOF.
 * @param servers Destination vector where parsed servers are appended.
 */
void Parser::parse(std::vector<ServerConfig> &servers)
{
	while (_currentToken().type != TOKEN_EOF)
	{
		if (_currentToken().type == TOKEN_KEYWORD && _currentToken().content == "server")
		{
			_advanceToken();
			ServerConfig newServer = _parseServerBlock();
			servers.push_back(newServer);
		}
		else
			throw std::runtime_error("Syntax error at line " /* + line number */ ": Expected 'server' block");
	}
}

/**
 * @brief Parse one server block and apply server directive handlers.
 * @return Parsed ServerConfig object.
 */
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

/**
 * @brief Parse one location block and apply location directive handlers.
 * @param path Location route/path token value.
 * @return Parsed LocationConfig object.
 */
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

/**
 * @brief Parse `listen` and set server host/port.
 * @param server Server configuration being populated.
 */
void Parser::_serverListen(ServerConfig &server)
{
	std::string listen;
	_extractSingleKeyword(listen);
	size_t colonPos = listen.find(':');
	if (colonPos != std::string::npos)
	{
		server.listenAddr = listen;
		server.host = listen.substr(0, colonPos);
		server.port = std::atoi(listen.substr(colonPos + 1).c_str());
	}
	else
	{
		server.listenAddr = "0.0.0.0:" + listen;
		server.host = "0.0.0.0";
		server.port = std::atoi(listen.c_str());
	}
	_expect(TOKEN_SEMICOLON);
	_validate_Listen(server.host, server.port);
}

/**
 * @brief Parse `server_name` values.
 * @param server Server configuration being populated.
 */
void Parser::_serverName(ServerConfig &server)
{
	_extractKeywordVector(server.serverNames);
	_expect(TOKEN_SEMICOLON);
	_validate_ServerNames(server.serverNames);
	//	*	ADD: _validate_NameServer_collision();
}

/**
 * @brief Parse `client_max_body_size`.
 * @param server Server configuration being populated.
 */
void Parser::_serverMaxBodySize(ServerConfig &server)
{
	std::string sizeValue;
	_extractSingleKeyword(sizeValue);
	bool isMegaByte = false;
	if (!sizeValue.empty() && sizeValue[sizeValue.size() - 1] == 'M')
	{
		isMegaByte = true;
		sizeValue.erase(sizeValue.size() - 1);
	}
	server.clientMaxBodySize = std::strtoul(sizeValue.c_str(), NULL, 10);
	if (isMegaByte)
		server.clientMaxBodySize *= 1048576;
	_expect(TOKEN_SEMICOLON);
	_validate_MaxBodySize(server.clientMaxBodySize);
}

/**
 * @brief Parse `error_page` mappings.
 * @param server Server configuration being populated.
 */
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
	_validate_ErrorPages(server.errorPages);
}

/**
 * @brief Parse nested `location` block under a server.
 * @param server Server configuration being populated.
 */
void Parser::_serverLocation(ServerConfig &server)
{
	t_token pathToken = _expect(TOKEN_KEYWORD);
	LocationConfig newLocation = _parseLocationBlock(pathToken.content);
	server.locations.push_back(newLocation);
}

//*	----- LocationHandler functions ----- */

/**
 * @brief Parse location `root` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationRoot(LocationConfig &location)
{
	_extractSingleKeyword(location.root);
	_expect(TOKEN_SEMICOLON);
}

/**
 * @brief Parse location `index` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationIndex(LocationConfig &location)
{
	_extractSingleKeyword(location.index);
	_expect(TOKEN_SEMICOLON);
}

/**
 * @brief Parse location `autoindex` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationAutoIndex(LocationConfig &location)
{
	std::string autoindex;
	_extractSingleKeyword(autoindex);
	if (autoindex != "on" && autoindex != "off")
		throw std::runtime_error("Syntax error: autoindex must be on/off");
	location.autoindex = autoindex == "on" ?  true : false;
	_expect(TOKEN_SEMICOLON);
}

/**
 * @brief Parse location `allow_methods` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationAllowMethods(LocationConfig &location)
{
	while (_currentToken().type == TOKEN_KEYWORD)
	{
		t_method method = HTTP::getMethod(_currentToken().content);
		location.allowedMethods.push_back(method);
		_advanceToken();
	}
	_expect(TOKEN_SEMICOLON);
	_validate_AllowedMethods(location.allowedMethods);
}

/**
 * @brief Parse location `return` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationReturn(LocationConfig &location)
{
	std::string returnCode;
	_extractSingleKeyword(returnCode);
	location.returnCode = std::atoi(returnCode.c_str());
	_extractSingleKeyword(location.returnUrl);
	_expect(TOKEN_SEMICOLON);
}

/**
 * @brief Parse location `cgi` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationCgi(LocationConfig &location)
{
	std::string ext;
	std::string exec;
	_extractSingleKeyword(ext);
	_extractSingleKeyword(exec);
	location.cgi[ext] = exec;
	_expect(TOKEN_SEMICOLON);
}

/**
 * @brief Parse location `upload_store` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationUploadStore(LocationConfig &location)
{
	_extractSingleKeyword(location.uploadStore);
	_expect(TOKEN_SEMICOLON);
}

//*	----- Helpers ----- */

/**
 * @brief Consume token if it matches expected type.
 * @param expected_type Expected token type.
 * @return Consumed token.
 * @throw std::runtime_error On mismatch or unexpected EOF.
 */
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

/**
 * @brief Get current token without consuming it.
 * @return Reference to current token.
 * @throw std::runtime_error If cursor is out of range.
 */
const t_token &Parser::_currentToken(void) const
{
	if (this->_cursor >= this->_tokens.size())
		throw std::runtime_error("Parser error: Unexpected end of tokens");
	return (this->_tokens[this->_cursor]);
}

/**
 * @brief Advance parser cursor by one token when possible.
 */
void Parser::_advanceToken(void)
{
	if (this->_cursor < this->_tokens.size())
		this->_cursor++;
}

/**
 * @brief Extract one keyword token content into destination.
 * @param destination Output string.
 */
void Parser::_extractSingleKeyword(std::string &destination)
{
	t_token token = _expect(TOKEN_KEYWORD);
	destination = token.content;
}

/**
 * @brief Extract all consecutive keyword token contents.
 * @param destination Output vector to append keyword contents into.
 */
void Parser::_extractKeywordVector(std::vector<std::string> &destination)
{
	while (_currentToken().type == TOKEN_KEYWORD)
	{
		destination.push_back(_currentToken().content);
		_advanceToken();
	}
}


//*	----- Server Validation ----- */

void Parser::_validate_Listen(const std::string &host, const int port)
{
	if (port < 1 || port > 65535)
		throw std::runtime_error("Config error: IP"); // ! Write error
	if (std::count(host.begin(), host.end(), '.') != 3)
		throw std::runtime_error("Config error: IP"); // ! Write error
	if (host == "localhost" || host == "0.0.0.0")
		return ;

	std::string octet;
	std::stringstream ss(host);
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

void Parser::_validate_ServerNames(const std::vector<std::string> &serverNames)
{
	if (serverNames.empty())
		throw std::runtime_error("Syntax error: server_name directive is empty");
	for (size_t i = 0; i < serverNames.size(); ++i)
	{
		const std::string &currentName = serverNames[i];
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

void Parser::_validate_MaxBodySize(const size_t clientMaxBodySize)
{
	if (clientMaxBodySize < 1 || clientMaxBodySize > ServerConfig::MAX_CLIENT_MAX_BODY_SIZE)
		throw std::runtime_error("Config error: Client Max Body Size"); // ! Write error
}

void Parser::_validate_ErrorPages(const std::map<t_status_code, std::string> &errorPages)
{
	std::map<t_status_code, std::string>::const_iterator it;
	for (it = errorPages.begin(); it != errorPages.end(); ++it)
	{
		int code = it->first;
		if (code < 400 || code > 599)
			throw std::runtime_error("Config error: Client Max Body Size"); // ! Write error
	}
}

void Parser::_validate_NameServer_Collision(const ServerConfig &server, const std::vector<std::string> &claimedNames)
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

//*	----- Location Validation ----- */

void Parser::_validate_AllowedMethods(const std::vector<t_method> &allowedMethods)
{
	// * CHECK REPEATED METHODS
	if (allowedMethods.empty())
		throw std::runtime_error("Syntax error: allow_methods directive is empty"); // ! Write error
	for (size_t i = 0; i < allowedMethods.size(); ++i)	
	{
		const t_method &currentMethod = allowedMethods[i];
		if (currentMethod == UNSUPPORTED_METHOD)
			throw std::runtime_error("Config error: Location - Allowed Methods"); // ! Write error
	}
}


void Parser::_add_to_ClaimedNames(const ServerConfig &server, std::vector<std::string> &dest)
{
	dest.insert(dest.end(), server.serverNames.begin(), server.serverNames.end());
}