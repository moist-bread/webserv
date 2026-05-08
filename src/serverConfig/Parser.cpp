#include "../../inc/serverConfig/Parser.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

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
			_expect(TOKEN_LBRACE);
			ServerConfig newServer;
			_parseServerBlock(newServer);
			servers.push_back(newServer);
		}
		else
			throw std::runtime_error("Syntax error at line " /* + line number */ ": Expected 'server' block");
	}
}

//*	----- ServerHandler functions ----- */

/**
 * @brief Parse one server block and apply server directive handlers.
 * @return Parsed ServerConfig object.
 */
void Parser::_parseServerBlock(ServerConfig &newServer)
{
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
}

//*	Listen
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
		server.port = stringToNumber<int>(listen.substr(colonPos + 1));
	}
	else
	{
		server.listenAddr = "0.0.0.0:" + listen;
		server.host = "0.0.0.0";
		server.port = stringToNumber<int>(listen);
	}
	_expect(TOKEN_SEMICOLON);
	_validate_Listen(server.host, server.port);
}

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
		int octet_value = stringToNumber<int>(octet);
		if (octet_value < 0 || octet_value > 255)
			throw std::runtime_error("Config error: IP"); // ! Write error
	}	
}
//*	----------- *

//* Server Names
/**
 * @brief Parse `server_name` values.
 * @param server Server configuration being populated.
 */
void Parser::_serverName(ServerConfig &server)
{
	_extractKeywordVector(server.serverNames);
	_expect(TOKEN_SEMICOLON);
	_validate_ServerNames(server.serverNames);
	//	!	ADD: _validate_NameServer_collision();
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
//*	----------- *

//* Max Body Size
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
	server.clientMaxBodySize = stringToNumber<long>(sizeValue);
	if (isMegaByte)
		server.clientMaxBodySize *= 1048576;
	_expect(TOKEN_SEMICOLON);
	_validate_MaxBodySize(server.clientMaxBodySize);
}

void Parser::_validate_MaxBodySize(const size_t clientMaxBodySize)
{
	if (clientMaxBodySize < 1 || clientMaxBodySize > ServerConfig::MAX_CLIENT_MAX_BODY_SIZE)
		throw std::runtime_error("Config error: Client Max Body Size"); // ! Write error
}
//*	----------- *

//* Error Page
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
		int errorCode = stringToNumber<int>(tempArgs[i]);
		server.errorPages[static_cast <t_status_code>(errorCode)] = uri;
	}
	_validate_ErrorPages(server.errorPages);
}

void Parser::_validate_ErrorPages(const std::map<t_status_code, std::string> &errorPages)
{
	std::map<t_status_code, std::string>::const_iterator it;
	for (it = errorPages.begin(); it != errorPages.end(); ++it)
	{
		int code = it->first;
		if (code < 400 || code > 599)
			throw std::runtime_error("Config error: Error Pages"); // ! Write error
	}
}
//*	----------- *

//*	----- LocationHandler functions ----- */

//* Location
//* Path
/**
 * @brief Parse nested `location` block under a server.
 * @param server Server configuration being populated.
 */
void Parser::_serverLocation(ServerConfig &server)
{
	t_token pathToken = _expect(TOKEN_KEYWORD);
	LocationConfig newLocation;
	newLocation.path = pathToken.content;
	_validate_Path(newLocation.path);
	_expect(TOKEN_LBRACE);
	_parseLocationBlock(newLocation);
	server.locations.push_back(newLocation);
}

void Parser::_validate_Path(const std::string &path)
{
	_isValidURI(path);
}
//*	----------- *

/**
 * @brief Parse one location block and apply location directive handlers.
 * @param path Location route/path token value.
 * @return Parsed LocationConfig object.
 */
void Parser::_parseLocationBlock(LocationConfig &newLocation)
{
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
}

//* Root
/**
 * @brief Parse location `root` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationRoot(LocationConfig &location)
{
	_extractSingleKeyword(location.root);
	_expect(TOKEN_SEMICOLON);
	// ! _validate_Root(location.root);		->	remove comment
}

void Parser::_validate_Root(const std::string &root)
{
	_isValidDirectory(root);
}
//*	----------- *

//* Index
/**
 * @brief Parse location `index` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationIndex(LocationConfig &location)
{
	location.index.pop_back();
	_extractKeywordVector(location.index);
	_expect(TOKEN_SEMICOLON);
	_validate_Index(location.index);
}

void Parser::_validate_Index(const std::vector<std::string> &index)
{
	for (size_t i = 0; i < index.size(); ++i)
	{
		if (!index[i].empty() && index[i][0] == '/')
			throw std::runtime_error("Syntax error: Invalid index");
		if (!index[i].empty() && index[i][index[i].length() - 1] == '/')
			throw std::runtime_error("Syntax error: Invalid index");
	}
}
//*	----------- *

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

//* Allow Methods
/**
 * @brief Parse location `allow_methods` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationAllowMethods(LocationConfig &location)
{
	location.allowedMethods.pop_back();
	while (_currentToken().type == TOKEN_KEYWORD)
	{
		t_method method = HTTP::getMethod(_currentToken().content);
		location.allowedMethods.push_back(method);
		_advanceToken();
	}
	_expect(TOKEN_SEMICOLON);
	_validate_AllowedMethods(location.allowedMethods);
}

void Parser::_validate_AllowedMethods(const std::vector<t_method> &allowedMethods)
{
	if (allowedMethods.empty())
		throw std::runtime_error("Syntax error: allow_methods directive is empty"); // ! Write error
	for (size_t i = 0; i < allowedMethods.size(); ++i)	
	{
		if (allowedMethods[i] == UNSUPPORTED_METHOD)
			throw std::runtime_error("Config error: Location - Unsuported Allowed Methods"); // ! Write error
		for (size_t j = 0; j < allowedMethods.size(); ++j)
		{
			if (j != i && allowedMethods[j] == allowedMethods[i])
				throw std::runtime_error("Config error: Location - Duplicated Allowed Methods"); // ! Write error
		}
	}
}
//*	----------- *

//* Return
/**
 * @brief Parse location `return` directive.
 * @param location Location configuration being populated.
 */
void Parser::_locationReturn(LocationConfig &location)
{
	std::string keyword;
	_extractSingleKeyword(keyword);
	int	returnCode = stringToNumber<int>(keyword);
	location.returnCode = HTTP::isValidReasonPhrase(returnCode);
	_extractSingleKeyword(location.returnUrl);
	_expect(TOKEN_SEMICOLON);
	_validate_ReturnCode(location.returnCode, location.returnUrl);
}

void Parser::_validate_ReturnCode(const t_status_code returnCode, const std::string &returnURL)
{
	if (returnCode == NO_STATUS)
		throw std::runtime_error("Config error: Location - Invalid code"); // ! Write error
	if (returnCode < 300 || returnCode > 308)
		throw std::runtime_error("Config error: Location - Unsuported return code"); // ! Write error
	_isValidURL(returnURL);	
}
//*	----------- *

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

//?	Needed ?
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

void Parser::_isValidURI(const std::string &uri) const
{
	if (uri.empty() || uri[0] != '/')
		throw std::runtime_error("Parser error: Invalid URI");
}

void Parser::_isValidURL(const std::string &url) const
{
	if (url.empty())
		throw std::runtime_error("Parser error: empty URL");
	if (url.length() >= 7 && url.compare(0, 7, "http://") == 0)
	{
		if (url.length() == 7)
			throw std::runtime_error("Parser error: Invalid URL format");
	}
	else if (url.length() >= 8 && url.compare(0, 8, "https://") == 0)
	{
		if (url.length() == 8)
			throw std::runtime_error("Parser error: Invalid URL format");
	}
	else if (url[0] != '/')
		throw std::runtime_error("Parser error: Invalid URL format");
}

void Parser::_isValidDirectory(const std::string &path) const
{
	if (access(path.c_str(), F_OK | R_OK | X_OK) != 0) 
		throw std::runtime_error("Parser error: Failed Path to Directory");

	struct stat path_stat;
	if (stat(path.c_str(), &path_stat) != 0)
		throw std::runtime_error("Parser error: Failed Path to Directory");
	if (!S_ISDIR(path_stat.st_mode))
		throw std::runtime_error("Parser error: Path is not a Directory");
}

void Parser::_add_to_ClaimedNames(const ServerConfig &server, std::vector<std::string> &dest)
{
	dest.insert(dest.end(), server.serverNames.begin(), server.serverNames.end());
}




//	! VERIFY ALL THE VARIABLES THAT ONLY ACCEPT NUMBERS

//	! CHECK DUPLICATE LOCATIONS

//	! CHECK DUPLCIATE SERVER_NAMES

//	! CHECK 'MUST HAVE' VARIABLES

/*

* 2. The Validation Checklist
A. Prefix Redundancy
	Just like you checked for server_name collisions, you must ensure you don't have two identical location paths in the same server.
		Check: Is there another location /html in this same server block? If yes, throw an error.

// B. Path & Root Inheritance
	The Logic: If loc.root is empty, do loc.root = parent.root;.
	The Check: After inheritance, if loc.root is still empty, the server literally won't know where to look for files. Throw error.

// C. Allowed Methods (The Security Gate)
	The 42 subject usually requires GET, POST, and DELETE.
		The Default: If the user didn't specify any methods, NGINX defaults to GET only.
		The Check: Loop through the methods. If you see PURGE or CONNECT, throw an error.

// D. Redirection (return)
	The Check: If a return directive exists, the status code MUST be a redirection code ($300$–$399$).
	The Pair: If they provided a code, they must provide a URL to redirect to.

E. The CGI Map
	The Check: For every entry in the cgi map (e.g., .php -> /usr/bin/php-cgi):
    	1. The extension must start with a dot.
   		2. The path to the executable must be valid (and ideally, you can use access(path, X_OK) to see if it's actually executable).

*/