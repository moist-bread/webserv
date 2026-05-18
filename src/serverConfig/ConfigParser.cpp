#include "../../inc/serverConfig/ConfigParser.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <cctype>

/**
 * @brief Construct Configparser and initialize directive dispatch tables.
 * @param tokens Immutable stream of lexer tokens.
 */
ConfigParser::ConfigParser(const std::vector<t_token> &tokens) : _ts(tokens)
{
	_serverHandlers["listen"] = &ConfigParser::_serverListen;
	_serverHandlers["server_name"] = &ConfigParser::_serverName;
	_serverHandlers["root"] = &ConfigParser::_serverRoot;
	_serverHandlers["client_max_body_size"] = &ConfigParser::_serverMaxBodySize;
	_serverHandlers["error_page"] = &ConfigParser::_serverErrorPage;

	_locationHandlers["root"] = &ConfigParser::_locationRoot;
	_locationHandlers["index"] = &ConfigParser::_locationIndex;
	_locationHandlers["autoindex"] = &ConfigParser::_locationAutoIndex;
	_locationHandlers["allow_methods"] = &ConfigParser::_locationAllowMethods;
	_locationHandlers["return"] = &ConfigParser::_locationReturn;
	_locationHandlers["cgi"] = &ConfigParser::_locationCgi;
	_locationHandlers["upload_store"] = &ConfigParser::_locationUploadStore;
}

/**
 * @brief Destroy Configparser instance.
 */
ConfigParser::~ConfigParser(void) {}

/**
 * @brief Parse all top-level server blocks until EOF.
 * @param servers Destination vector where parsed servers are appended.
 */
std::vector<ServerConfig> ConfigParser::parse(void)
{
	std::vector<ServerConfig> newServers;
	while (_ts._currentToken().type != TOKEN_EOF)
	{
		if (_ts._currentToken().type == TOKEN_KEYWORD && _ts._currentToken().content == "server")
		{
			_ts._advanceToken();
			_ts._expect(TOKEN_LBRACE);
			ServerConfig newServer;
			_parseServerBlock(newServer);
			_finalizeServer(newServer);
			newServers.push_back(newServer);
		}
		else
			_ts.throwSyntaxError("Expected 'server' block", _ts._currentToken().line);
	}
	_validate_ServerCollision(newServers);
	return (newServers);
}

void ConfigParser::_finalizeServer(ServerConfig &server)
{
	//	Listen
	if (server.listen.string.empty())
	{
		server.listen.host = "0.0.0.0";
		server.listen.port = 8080;
		server.listen.string = "0.0.0.0:8080";
	}
	//?	Server Names -> keep it empty
	//?	Server Root -> keep it empty
	//	Client Max Body Size
	if (server.clientMaxBodySize == 0)
		server.clientMaxBodySize = ServerConfig::DEFAULT_CLIENT_MAX_BODY_SIZE;
	//	Locations
	if (server.locations.empty())
		_ts.throwValidationError("No Location", "server");
	_finalizeLocation(server);
}

void ConfigParser::_finalizeLocation(ServerConfig &server)
{
	for (size_t i = 0; i < server.locations.size(); ++i)
	{
		LocationConfig &location = server.locations[i];
		if (location.returnCode == UNITIALIZED)
			location.returnCode = NO_STATUS;
		if (location.root.empty())
		{
			if (location.returnCode == NO_STATUS)
			{
				if (server.root.empty())
					_ts.throwValidationError("No Location Root, couldnt inherit from Server Root(not inserted)", "location");
				else
					location.root = server.root;
			}
		}
		if (location.index.empty())
			location.index.push_back("index.html");
		if (location.autoindex == -1)
			location.autoindex = false;
		if (location.allowedMethods.empty())
		{
			location.allowedMethods.push_back(GET);
			location.allowedMethods.push_back(POST);
			location.allowedMethods.push_back(DELETE);
		}
		//?	Upload Store -> keep it empty - server doesnt allow uploads
		//?	Return URL -> keep it empty - no Status code
		//?	CGI -> keep it empy - server doesnt execute scripts
	}
}

//*	----- ServerHandler functions ----- */

/**
 * @brief Parse one server block and apply server directive handlers.
 * @return Parsed ServerConfig object.
 */
void ConfigParser::_parseServerBlock(ServerConfig &newServer)
{
	std::set<std::string> locationsPathRecord;
	while (_ts._currentToken().type != TOKEN_RBRACE)
	{
		t_token directiveToken = _ts._expect(TOKEN_KEYWORD);
		std::string directive = directiveToken.content;
		if (directive == "location")
			_serverLocation(newServer, locationsPathRecord);
		else if (_serverHandlers.count(directive) > 0)
		{
			ServerHandler handler = _serverHandlers[directive];
			(this->*handler)(newServer);
		}
		else
			_ts.throwSyntaxError("Unknown directive '" + directiveToken.content + "'", directiveToken.line);
	}
	_ts._expect(TOKEN_RBRACE);
}

void ConfigParser::_validate_ServerCollision(const std::vector<ServerConfig> &servers)
{
	for (size_t i = 0; i < servers.size(); ++i)
	{
		const ServerConfig &server_A = servers[i];
		for (size_t j = i + 1; j < servers.size(); ++j)
		{
			const ServerConfig &server_B = servers[j];
			if (server_A.listen.string == server_B.listen.string)
			{
				_ts.throwValidationError("Server Name/Listen Address collision between servers", "server");
				_validate_ServerNamesCollision(server_A, server_B);
			}
		}
	}
}

void ConfigParser::_validate_ServerNamesCollision(const ServerConfig &server_A, const ServerConfig &server_B)
{
	for (size_t i = 0; i < server_A.serverNames.size(); ++i)
	{
		for (size_t j = 0; j < server_B.serverNames.size(); ++j)
		{
			if (server_A.serverNames[i] == server_B.serverNames[j])
				_ts.throwValidationError("Server Name/Listen Address collision between servers", "server_name");
		}
	}
}


//*	Listen
/**
 * @brief Parse `listen` and set server host/port.
 * @param server Server configuration being populated.
 */
void ConfigParser::_serverListen(ServerConfig &server)
{
	if (!server.listen.string.empty())
		_ts.throwValidationError("Listen duplicated", "listen");
	std::string listen;
	_ts._extractSingleKeyword(listen);
	size_t colonPos = listen.find(':');
	if (colonPos != std::string::npos)
	{
		server.listen.string = listen;
		server.listen.host = listen.substr(0, colonPos);
		server.listen.port = stringToNumber<int>(listen.substr(colonPos + 1));
	}
	else
	{
		server.listen.string = "0.0.0.0:" + listen;
		server.listen.host = "0.0.0.0";
		server.listen.port = stringToNumber<int>(listen);
	}
	_validate_Listen(server.listen.host, server.listen.port);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_Listen(const std::string &host, const int port)
{
	if (port < 1 || port > 65535)
		_ts.throwValidationError("IP", "listen");
	if (host == "localhost" || host == "0.0.0.0")
		return ;
	if (std::count(host.begin(), host.end(), '.') != 3)
		_ts.throwValidationError("IP", "listen");

	std::string octet;
	std::stringstream ss(host);
	while (std::getline(ss, octet, '.'))
	{
		if (octet.empty())
			_ts.throwValidationError("IP", "listen");
		for (size_t i = 0; i < octet.length(); i++)
		{
			if (!std::isdigit(octet[i]))
				_ts.throwValidationError("IP", "listen");
		}
		int octet_value = stringToNumber<int>(octet);
		if (octet_value < 0 || octet_value > 255)
			_ts.throwValidationError("IP", "listen");
	}	
}
//*	----------- *

//* Server Names
/**
 * @brief Parse `server_name` values.
 * @param server Server configuration being populated.
 */
void ConfigParser::_serverName(ServerConfig &server)
{
	_ts._extractKeywordVector(server.serverNames);
	_validate_ServerNames(server.serverNames);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_ServerNames(const std::vector<std::string> &serverNames)
{
	if (serverNames.empty())
		_ts.throwSyntaxError("server_name directive is empty", _ts._currentToken().line);
	for (size_t i = 0; i < serverNames.size(); ++i)
	{
		const std::string &currentName = serverNames[i];
		if (currentName == "_")
			continue ;
		if (currentName[0] == '.' || currentName[0] == '-' || 
				currentName[currentName.length() - 1] == '.' ||
				currentName[currentName.length() - 1] == '-')
			_ts.throwValidationError("Server Names", "server_name");
		for (size_t j = 0; j < currentName.length(); ++j)
		{
			char c = currentName[j];
			if (!std::isalnum(c) && c != '.' && c != '-')
				_ts.throwValidationError("Server Names", "server_name");
			if (c == '.' && currentName[j - 1] == '.')
				_ts.throwValidationError("Server Names", "server_name");
		}
	}
}
//*	----------- *

//* Root
void ConfigParser::_serverRoot(ServerConfig &server)
{
	if (!server.root.empty())
		_ts.throwValidationError("Root duplicated", "root");
	_ts._extractSingleKeyword(server.root);
	_validate_Root(server.root);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_Root(const std::string &root)
{
	_isValidDirectory(root, R_OK | X_OK);
}
//*	----------- *

//* Max Body Size
/**
 * @brief Parse `client_max_body_size`.
 * @param server Server configuration being populated.
 */
void ConfigParser::_serverMaxBodySize(ServerConfig &server)
{
	if (server.clientMaxBodySize != 0)
		_ts.throwValidationError("Client Max Body Size duplicated", "client_max_body_size");
	std::string sizeValue;
	_ts._extractSingleKeyword(sizeValue);
	bool isMegaByte = false;
	if (!sizeValue.empty() && sizeValue[sizeValue.size() - 1] == 'M')
	{
		isMegaByte = true;
		sizeValue.erase(sizeValue.size() - 1);
	}
	server.clientMaxBodySize = stringToNumber<long>(sizeValue);
	if (server.clientMaxBodySize == 0)
		_ts.throwValidationError("Client Max Body Size cant be 0", "client_max_body_size");
	if (isMegaByte)
		server.clientMaxBodySize *= 1048576;
	_validate_MaxBodySize(server.clientMaxBodySize);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_MaxBodySize(const size_t clientMaxBodySize)
{
	if (clientMaxBodySize < 1 || clientMaxBodySize > ServerConfig::MAX_CLIENT_MAX_BODY_SIZE)
		_ts.throwValidationError("Client Max Body Size", "client_max_body_size");
}
//*	----------- *

//* Error Page
/**
 * @brief Parse `error_page` mappings.
 * @param server Server configuration being populated.
 */
void ConfigParser::_serverErrorPage(ServerConfig &server)
{
	std::vector<std::string> tempArgs;
	_ts._extractKeywordVector(tempArgs);
	if (tempArgs.size() < 2)
	      	_ts.throwValidationError("error_page needs at least one code and a URI", "error_page");
	std::string uri = tempArgs.back();
	tempArgs.pop_back();
	for (size_t i = 0; i < tempArgs.size(); ++i)
	{
		int errorCode = stringToNumber<int>(tempArgs[i]);
		server.errorPages[static_cast <t_status_code>(errorCode)] = uri;
	}
	_validate_ErrorPages(server.errorPages);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_ErrorPages(const std::map<t_status_code, std::string> &errorPages)
{
	std::map<t_status_code, std::string>::const_iterator it;
	for (it = errorPages.begin(); it != errorPages.end(); ++it)
	{
		int code = it->first;
		if (code < 400 || code > 599)
			_ts.throwValidationError("Error Pages", "error_page");
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
void ConfigParser::_serverLocation(ServerConfig &server, std::set<std::string> &locationsPathRecord)
{
	t_token pathToken = _ts._expect(TOKEN_KEYWORD);

	LocationConfig newLocation;
	newLocation.path = pathToken.content;
	_validate_Path(newLocation.path, locationsPathRecord);
	_ts._expect(TOKEN_LBRACE);

	_parseLocationBlock(newLocation);
	server.locations.push_back(newLocation);
}

void ConfigParser::_validate_Path(const std::string &path, std::set<std::string> &locationsPathRecord)
{
	if (!_isValidURI(path))
		_ts.throwValidationError("Invalid URI", "location");
	if (locationsPathRecord.insert(path).second == false)
		_ts.throwValidationError("Duplicated Location Path", "location");
}
//*	----------- *

/**
 * @brief Parse one location block and apply location directive handlers.
 * @param path Location route/path token value.
 * @return Parsed LocationConfig object.
 */
void ConfigParser::_parseLocationBlock(LocationConfig &newLocation)
{
	while (_ts._currentToken().type != TOKEN_RBRACE)
	{
		t_token directiveToken = _ts._expect(TOKEN_KEYWORD);
		std::string directive = directiveToken.content;
		if (_locationHandlers.count(directive) > 0)
		{
			LocationHandler handler = _locationHandlers[directive];
			(this->*handler)(newLocation);
		}
		else
			_ts.throwSyntaxError("Unknown locationdirective '" + directiveToken.content + "'", directiveToken.line);
	}
	_ts._expect(TOKEN_RBRACE);
}

//* Root
/**
 * @brief Parse location `root` directive.
 * @param location Location configuration being populated.
 */
void ConfigParser::_locationRoot(LocationConfig &location)
{
	if (!location.root.empty())
		_ts.throwValidationError("Root duplicated", "root");
	_ts._extractSingleKeyword(location.root);
	_validate_Root(location.root);
	_ts._expect(TOKEN_SEMICOLON);
}
//*	----------- *

//* Index
/**
 * @brief Parse location `index` directive.
 * @param location Location configuration being populated.
 */
void ConfigParser::_locationIndex(LocationConfig &location)
{
	_ts._extractKeywordVector(location.index);
	_validate_Index(location.index);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_Index(const std::vector<std::string> &index)
{
	for (size_t i = 0; i < index.size(); ++i)
	{
		if (!index[i].empty() && index[i][0] == '/')
			_ts.throwSyntaxError("Invalid index", _ts._currentToken().line);
		if (!index[i].empty() && index[i][index[i].length() - 1] == '/')
			_ts.throwSyntaxError("Invalid index", _ts._currentToken().line);
	}
}
//*	----------- *

//* Auto Index
/**
 * @brief Parse location `autoindex` directive.
 * @param location Location configuration being populated.
 */
void ConfigParser::_locationAutoIndex(LocationConfig &location)
{
	if (location.autoindex != -1)
		_ts.throwValidationError("Auto Index duplicated", "autoindex");
	std::string autoindex;
	_ts._extractSingleKeyword(autoindex);
	if (autoindex != "on" && autoindex != "off")
		_ts.throwSyntaxError("autoindex must be on/off", _ts._currentToken().line);
	location.autoindex = autoindex == "on" ?  true : false;
	_ts._expect(TOKEN_SEMICOLON);
}
//*	----------- *

//* Allow Methods
/**
 * @brief Parse location `allow_methods` directive.
 * @param location Location configuration being populated.
 */
void ConfigParser::_locationAllowMethods(LocationConfig &location)
{
	while (_ts._currentToken().type == TOKEN_KEYWORD)
	{
		t_method method = HTTP::getMethod(_ts._currentToken().content);
        if (std::find(location.allowedMethods.begin(), location.allowedMethods.end(), method) == location.allowedMethods.end())
			location.allowedMethods.push_back(method);
		_ts._advanceToken();
	}
	_validate_AllowedMethods(location.allowedMethods);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_AllowedMethods(const std::vector<t_method> &allowedMethods)
{
	if (allowedMethods.empty())
		_ts.throwValidationError("allow_methods directive is empty", "allow_methods");
	for (size_t i = 0; i < allowedMethods.size(); ++i)	
	{
		if (allowedMethods[i] == UNSUPPORTED_METHOD)
			_ts.throwValidationError("Unsuported Allowed Methods", "allow_methods");
	}
}
//*	----------- *

//* Return
/**
 * @brief Parse location `return` directive.
 * @param location Location configuration being populated.
 */
void ConfigParser::_locationReturn(LocationConfig &location)
{
	if (location.returnCode != UNITIALIZED)
		_ts.throwValidationError("Return duplicated", "return");
	std::string keyword;
	_ts._extractSingleKeyword(keyword);
	int	returnCode = stringToNumber<int>(keyword);
	location.returnCode = HTTP::isValidReasonPhrase(returnCode);
	_ts._extractSingleKeyword(location.returnUrl);
	_validate_ReturnCode(location.returnCode, location.returnUrl);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_ReturnCode(const t_status_code returnCode, const std::string &returnURL)
{
	if (returnCode == NO_STATUS)
		_ts.throwValidationError("Invalid code", "return");
	if (returnCode < 300 || returnCode > 308)
		_ts.throwValidationError("Unsuported return code", "return");
	if (!_isValidURL(returnURL))
		_ts.throwValidationError("Invalid URL", "return");

}
//*	----------- *

//* CGI
/**
 * @brief Parse location `cgi` directive.
 * @param location Location configuration being populated.
 */
void ConfigParser::_locationCgi(LocationConfig &location)
{
	std::string ext;
	std::string exec;
	_ts._extractSingleKeyword(ext);
	_ts._extractSingleKeyword(exec);
	_validate_Cgi(ext, exec);	
	location.cgi[ext] = exec;
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_Cgi(const std::string &extension, const std::string &executer)
{
	if (extension.length() < 2 || extension[0] != '.')
		_ts.throwValidationError("Invalid CGI extension", "cgi");
	for (size_t i = 1; i < extension.length(); ++i)
	{
		if (!std::isalnum(extension[i]))
			_ts.throwValidationError("Invalid CGI extension", "cgi");
	}
	if (!_isValidFile(executer, R_OK | X_OK))
		_ts.throwValidationError("Invalid CGI executable", "cgi");
}
//*	----------- *

//* Upload Store
/**
 * @brief Parse location `upload_store` directive.
 * @param location Location configuration being populated.
 */
void ConfigParser::_locationUploadStore(LocationConfig &location)
{
	if (!location.uploadStore.empty())
		_ts.throwValidationError("Upload Store duplicated", "upload_store");
	_ts._extractSingleKeyword(location.uploadStore);
	_validate_UploadStore(location.uploadStore);
	_ts._expect(TOKEN_SEMICOLON);
}

void ConfigParser::_validate_UploadStore(const std::string &path)
{
	if (!_isValidDirectory(path, W_OK | X_OK))
		_ts.throwValidationError("Upload Store", "upload_store");
}
//*	----------- *

//*	----- Helpers ----- */

bool ConfigParser::_isValidURI(const std::string &uri) const
{
	if (uri.empty() || uri[0] != '/')
		return (false);
	return (true);
}

bool ConfigParser::_isValidURL(const std::string &url) const
{
	if (url.empty())
		return (false);
	if (url.length() >= 7 && url.compare(0, 7, "http://") == 0)
	{
		if (url.length() == 7)
			return (false);
	}
	else if (url.length() >= 8 && url.compare(0, 8, "https://") == 0)
	{
		if (url.length() == 8)
			return (false);
	}
	else if (url[0] != '/')
		return (false);
	return (true);
}

bool ConfigParser::_isValidAccess(const std::string &path, const int flags) const
{
	if (access(path.c_str(), flags) != 0)
		return (false);
	return (true);
}

bool ConfigParser::_isValidFile(const std::string &path, const int flags) const
{
	if (!_isValidAccess(path, flags))
		return (false);
	struct stat path_stat;
	if (stat(path.c_str(), &path_stat) != 0)
		return (false);
	if (S_ISDIR(path_stat.st_mode))
		return (false);
	return (true);
}

bool ConfigParser::_isValidDirectory(const std::string &path, const int flags) const
{
	if (!_isValidAccess(path, flags))
		return (false);
	struct stat path_stat;
	if (stat(path.c_str(), &path_stat) != 0)
		return (false);
	if (!S_ISDIR(path_stat.st_mode))
		return (false);
	return (true);
}


/*

	! CHECK ERROR MESSAGES -> NOTE TO ADD LINE NUMBER / *COLLUMN?*

	! CHECK FILE NAME EXTENSION -> LEXER

*/
