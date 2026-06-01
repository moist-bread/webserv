#include "../../inc/serverConfig/ConfigParser.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include <stdexcept>
#include <algorithm> // find
#include <sys/stat.h>
#include <unistd.h>
#include <cctype>

/**
 * @brief Construct a ConfigParser and initialize directive dispatch tables.
 *
 * The parser is constructed with an immutable vector of lexer tokens. It
 * initializes the internal `TokenStream` (`_ts`) with that token list and
 * registers server- and location-level directive handlers in the lookup
 * tables. Handlers are later dispatched when parsing `server` and
 * `location` blocks.
 *
 * @param tokens A vector of `t_token` produced by the `Lexer` for a single
 * configuration file. The parser does not take ownership but copies the
 * reference into its `TokenStream` wrapper which manages cursor state.
 */
ConfigParser::ConfigParser(const std::vector<t_token> &tokens) : _ts(tokens)
{
	_serverHandlers["listen"] = &ConfigParser::_serverListen;
	_serverHandlers["server_name"] = &ConfigParser::_serverName;
	_serverHandlers["root"] = &ConfigParser::_serverRoot;
	_serverHandlers["client_max_body_size"] = &ConfigParser::_serverMaxBodySize;
	_serverHandlers["error_page"] = &ConfigParser::_serverErrorPage;
	_serverHandlers["cgi"] = &ConfigParser::_serverCgi;

	_locationHandlers["root"] = &ConfigParser::_locationRoot;
	_locationHandlers["index"] = &ConfigParser::_locationIndex;
	_locationHandlers["autoindex"] = &ConfigParser::_locationAutoIndex;
	_locationHandlers["allow_methods"] = &ConfigParser::_locationAllowMethods;
	_locationHandlers["return"] = &ConfigParser::_locationReturn;
	_locationHandlers["cgi"] = &ConfigParser::_locationCgi;
	_locationHandlers["cgi_pass"] = &ConfigParser::_locationCgi;
	_locationHandlers["upload_store"] = &ConfigParser::_locationUploadStore;
}

/**
 * @brief Destroy the ConfigParser instance.
 *
 * Default destructor: no special cleanup required because all owned
 * resources are value-type members (e.g. `TokenStream`).
 */
ConfigParser::~ConfigParser(void) {}

/**
 * @brief Parse the token stream into server configurations.
 *
 * Iterates the token stream until `TOKEN_EOF`, expecting one or more
 * top-level `server { ... }` blocks. For each server block a
 * `ServerConfig` object is constructed, populated by dispatching directive
 * handlers and finalized via `_finalizeServer()` before being added to the
 * returned vector.
 *
 * On encountering unexpected tokens or invalid constructs a syntax or
 * validation exception is thrown via `TokenStream` helpers. After parsing
 * all servers, a cross-server validation `_validate_ServerCollision` is
 * executed to detect ambiguous listen/name configurations.
 *
 * @return Vector of fully-parsed and validated `ServerConfig` objects.
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

/**
 * @brief Finalize a parsed server configuration.
 *
 * Apply defaults and perform server-local sanity checks once all directives
 * were parsed for a particular `ServerConfig`. This includes:
 * - Defaulting `listen` to `127.0.0.1:8080` when not specified.
 * - Defaulting `clientMaxBodySize` to `ServerConfig::DEFAULT_CLIENT_MAX_BODY_SIZE`.
 * - Ensuring at least one `location` is present; otherwise a validation
 *   error is thrown.
 * - Finalizing each `LocationConfig` via `_finalizeLocation` which applies
 *   location-level defaults/inheritance.
 *
 * @param server ServerConfig instance to finalize (modified in-place).
 */
void ConfigParser::_finalizeServer(ServerConfig &server)
{
	//	Listen
	if (server.listen.string.empty())
	{
		server.listen.host = "127.0.0.1";
		server.listen.port = 8080;
		server.listen.string = "127.0.0.1:8080";
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

/**
 * @brief Apply defaults and validations to all locations of a server.
 *
 * Iterates `server.locations` and for each `LocationConfig` applies defaults
 * such as default `index`, default `autoindex` value, default allowed
 * methods (GET, POST, DELETE), and inheritance of `root` from the server
 * when appropriate. If required values are missing and cannot be inherited a
 * validation error is reported.
 *
 * @param server ServerConfig whose locations will be finalized (modified in-place).
 */
void ConfigParser::_finalizeLocation(ServerConfig &server)
{
	for (size_t i = 0; i < server.locations.size(); ++i)
	{
		LocationConfig &location = server.locations[i];
		if (location.CgiPass)
		{
			_finalizeLocationCgiPass(location, server);
			continue;
		}
		if (location.returnCode != INVALID_CODE)
		{
			_finalizeLocationReturn(location);
			continue;
		}
		if (location.root.empty())
		{
			if (server.root_default.empty())
				_ts.throwValidationError("No Location Root, couldnt inherit from Server Root (not inserted)", "location");
			else
				location.root = server.root_default;
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
		if (location.isMethodAllowed(POST) && location.uploadStore.empty())
			_ts.throwValidationError("No Location upload_store, won't be able to POST files", "upload_store");
		if (location.cgi.empty())
		{
			if (!server.cgi_default.empty())
				location.cgi = server.cgi_default;
		}
	}
}

void ConfigParser::_finalizeLocationCgiPass(LocationConfig &location, ServerConfig &server)
{
	if (location.cgi.empty())
		_ts.throwValidationError("Location with '*.extendion', must have cgi_pass declared", "location");
	if (location.allowedMethods.empty())
		_ts.throwValidationError("Location with cgi pass, must have Allowed Methods declared", "location");
	if (location.root.empty())
	{
		if (server.root_default.empty())
			_ts.throwValidationError("No Location Root, couldnt inherit from Server Root (not inserted)", "location");
		else
			location.root = server.root_default;
	}
	if (!location.index.empty())
		_ts.throwValidationError("Location with cgi pass, can't have index", "location");
	if (location.autoindex == 1)
		_ts.throwValidationError("Location with cgi pass, can't have autoindexing", "location");
	if (location.returnCode != INVALID_CODE)
		_ts.throwValidationError("Location with cgi pass, can't have return", "location");
	//! if (!location.uploadStore.empty())
	//! 	_ts.throwValidationError("Location with return, can't have upload_store", "location");
}

void ConfigParser::_finalizeLocationReturn(LocationConfig &location)
{
	if (!location.index.empty())
		_ts.throwValidationError("Location with return, can't have index", "location");
	if (!location.root.empty())
		_ts.throwValidationError("Location with return, can't have root", "location");
	if (!location.cgi.empty())
		_ts.throwValidationError("Location with return, can't have cgi", "location");
	if (!location.uploadStore.empty())
		_ts.throwValidationError("Location with return, can't have upload_store", "location");
	if (location.autoindex == 1)
		_ts.throwValidationError("Location with return, can't have autoindexing", "location");
}

//*	----- ServerHandler functions ----- */

/**
 * @brief Parse the contents of a `server { ... }` block.
 *
 * Reads directives until the matching right brace `}`. Each encountered
 * directive keyword is dispatched to the registered server handler table
 * (`_serverHandlers`). The special `location` directive is handled inline
 * because it consumes a nested block and requires path-uniqueness tracking
 * (`locationsPathRecord`). Unknown directives produce a syntax error.
 *
 * The function expects to be called after the opening `{` has been
 * consumed. It will consume the closing `}` before returning.
 *
 * @param newServer `ServerConfig` instance to populate (modified in-place).
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

/**
 * @brief Validate that servers do not collide on listen address/name.
 *
 * Performs pairwise comparison of parsed servers to detect servers that
 * share the same `listen.string`. If a collision is found `_ts.throwValidationError`
 * is invoked and `_validate_ServerNamesCollision` is called to provide more
 * granular name-collision diagnostics.
 *
 * @param servers Vector of parsed servers to validate.
 */
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
				_ts.throwValidationError("Collision between servers - shared listen: '" + server_A.listen.string + "'", "server");
				_validate_ServerNamesCollision(server_A, server_B);
			}
		}
	}
}

/**
 * @brief Check for duplicate `server_name` entries between two servers.
 *
 * Compares the `serverNames` vectors of `server_A` and `server_B`. If a
 * matching name is found a validation error is reported anchored to the
 * `server_name` directive.
 *
 * @param server_A First server to compare.
 * @param server_B Second server to compare.
 */
void ConfigParser::_validate_ServerNamesCollision(const ServerConfig &server_A, const ServerConfig &server_B)
{
	for (size_t i = 0; i < server_A.serverNames.size(); ++i)
	{
		for (size_t j = 0; j < server_B.serverNames.size(); ++j)
		{
			if (server_A.serverNames[i] == server_B.serverNames[j])
				_ts.throwValidationError("Collision between servers - shared server name: '" + server_A.serverNames[i] + "'", "server_name");
		}
	}
}


//*	Listen
/**
 * @brief Parse the `listen` directive and populate `server.listen`.
 *
 * Accepts forms like `IP:PORT` or `PORT`. When only a port is provided the
 * host defaults to `0.0.0.0`. The parsed host/port are validated via
 * `_validate_Listen`. Duplicate `listen` directives on the same server
 * produce a validation error. This function consumes the directive's
 * keyword token, its arguments and the terminating semicolon.
 *
 * @param server ServerConfig to populate (modified in-place).
 */
void ConfigParser::_serverListen(ServerConfig &server)
{
	if (!server.listen.string.empty())
		_ts.throwValidationError("Duplicated", "listen");
	std::string listen;
	_ts._extractSingleKeyword(listen);
	size_t colonPos = listen.find(':');
	try
	{
		if (colonPos != std::string::npos)
		{
			server.listen.string = listen;
			server.listen.host = listen.substr(0, colonPos);
			server.listen.port = stringToNumber<int>(listen.substr(colonPos + 1));
		}
		else
		{
			server.listen.string = "127.0.0.1:" + listen;
			server.listen.host = "127.0.0.1";
			server.listen.port = stringToNumber<int>(listen);
		}
	}
	catch(const std::exception& e) { _ts.throwValidationError(e.what(), "listen"); }
	
	_validate_Listen(server.listen.host, server.listen.port);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate the host and port values parsed from `listen`.
 *
 * Checks that the port is in the valid TCP range [1,65535]. For hostnames
 * that are neither `localhost` nor `0.0.0.0` a simple IPv4 dotted-quad
 * validation is performed (four numeric octets between 0 and 255).
 *
 * On invalid values a validation error is reported via `_ts.throwValidationError`.
 *
 * @param host Host string extracted from the `listen` directive.
 * @param port Port number extracted from the `listen` directive.
 */
void ConfigParser::_validate_Listen(const std::string &host, const int port)
{
	if (port < 1 || port > 65535)
		_ts.throwValidationError("Out of range", "listen");
	if (host == "localhost" || host == "127.0.0.1")
		return ;
	if (std::count(host.begin(), host.end(), '.') != 3)
		_ts.throwValidationError("invalid host format", "listen");

	std::string octet;
	std::stringstream ss(host);
	while (std::getline(ss, octet, '.'))
	{
		if (octet.empty())
			_ts.throwValidationError("invalid host format - empty host octet", "listen");
		for (size_t i = 0; i < octet.length(); i++)
		{
			if (!std::isdigit(octet[i]))
				_ts.throwValidationError("invalid host format - must be a digit", "listen");
		}
		int octet_value = -1;
		try { octet_value = stringToNumber<int>(octet); }
		catch(const std::exception& e) { _ts.throwValidationError(e.what(), "listen"); }

		if (octet_value < 0 || octet_value > 255)
			_ts.throwValidationError("invalid host - out of range octet", "listen");
	}	
}
//*	----------- *

//* Server Names
/**
 * @brief Parse the `server_name` directive into `server.serverNames`.
 *
 * Reads one or more names (space-separated) and stores them in the server
 * configuration. Names are validated for allowed characters and form via
 * `_validate_ServerNames`. Consumes the directive tokens and the
 * terminating semicolon.
 *
 * @param server ServerConfig to populate (modified in-place).
 */
void ConfigParser::_serverName(ServerConfig &server)
{
	_ts._extractKeywordVector(server.serverNames);
	_validate_ServerNames(server.serverNames);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate syntactic correctness of server name tokens.
 *
 * Ensures names are non-empty, do not begin or end with `.` or `-`, only
 * contain alphanumeric characters, dots or dashes, and do not contain
 * consecutive dots. The special single underscore `_` is allowed and
 * represents the default server name.
 *
 * On invalid names a validation or syntax error is thrown.
 *
 * @param serverNames Vector of server name strings to validate.
 */
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
			_ts.throwValidationError("invalid server name", "server_name");
		for (size_t j = 0; j < currentName.length(); ++j)
		{
			char c = currentName[j];
			if (!std::isalnum(c) && c != '.' && c != '-')
				_ts.throwValidationError("invalid server name", "server_name");
			if (c == '.' && currentName[j - 1] == '.')
				_ts.throwValidationError("invalid server name", "server_name");
		}
	}
}
//*	----------- *

//* Root
/**
 * @brief Parse the server-level `root` directive and validate it.
 *
 * Stores the provided filesystem path in `server.root`. Duplicate `root`
 * directives are rejected. The path is validated for existence and
 * permissions using `_validate_Root`. This function consumes the
 * terminating semicolon.
 *
 * @param server ServerConfig to populate (modified in-place).
 */
void ConfigParser::_serverRoot(ServerConfig &server)
{
	if (!server.root_default.empty())
		_ts.throwValidationError("Duplicated", "root");
	_ts._extractSingleKeyword(server.root_default);
	_validate_Root(server.root_default);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate that the `root` path exists and is a directory with
 * the required permissions.
 *
 * Delegates to `_isValidDirectory` to check filesystem attributes.
 *
 * @param root Filesystem path to validate.
 */
void ConfigParser::_validate_Root(std::string &root)
{
	if (root.length() > 2 && *(root.end() - 1) == '/')
		root.erase(root.end() - 1);
	try { _isValidDirectory(root, R_OK | X_OK); }
	catch (const std::exception &e) { _ts.throwValidationError(e.what(), "root"); }
}
//*	----------- *

//* Max Body Size
/**
 * @brief Parse the `client_max_body_size` directive.
 *
 * Accepts a numeric value optionally suffixed with `M` to indicate
 * megabytes (e.g. `10M`). The value is converted to bytes and validated
 * to be within allowed limits. Duplicate directives or zero values result
 * in validation errors.
 *
 * @param server ServerConfig to populate (modified in-place).
 */
void ConfigParser::_serverMaxBodySize(ServerConfig &server)
{
	if (server.clientMaxBodySize != 0)
		_ts.throwValidationError("Duplicated", "client_max_body_size");
	std::string sizeValue;
	_ts._extractSingleKeyword(sizeValue);
	bool isMegaByte = false;
	if (!sizeValue.empty() && sizeValue[sizeValue.size() - 1] == 'M')
	{
		isMegaByte = true;
		sizeValue.erase(sizeValue.size() - 1);
	}
	try { server.clientMaxBodySize = stringToNumber<long>(sizeValue); }
	catch(const std::exception& e) { _ts.throwValidationError(e.what(), "client_max_body_size"); }

	if (server.clientMaxBodySize == 0)
		_ts.throwValidationError("cant be 0", "client_max_body_size");
	if (isMegaByte)
		server.clientMaxBodySize *= 1048576;
	_validate_MaxBodySize(server.clientMaxBodySize);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate the configured `client_max_body_size` is within limits.
 *
 * Ensures the value is >= 1 and does not exceed
 * `ServerConfig::MAX_CLIENT_MAX_BODY_SIZE`. Throws a validation error on
 * invalid inputs.
 *
 * @param clientMaxBodySize Size in bytes to validate.
 */
void ConfigParser::_validate_MaxBodySize(const size_t clientMaxBodySize)
{
	if (clientMaxBodySize < 1 || clientMaxBodySize > ServerConfig::MAX_CLIENT_MAX_BODY_SIZE)
		_ts.throwValidationError("Out of range", "client_max_body_size");
}
//*	----------- *

//* Error Page
/**
 * @brief Parse the `error_page` directive mapping status codes to URIs.
 *
 * The directive accepts one or more numeric error codes followed by a
 * URI (e.g. `error_page 404 500 /error.html;`). At least one code and a
 * target URI are required. Each code is inserted into `server.errorPages`.
 * The collected map is validated via `_validate_ErrorPages`.
 *
 * @param server ServerConfig to populate (modified in-place).
 */
void ConfigParser::_serverErrorPage(ServerConfig &server)
{
	std::vector<std::string> tempArgs;
	_ts._extractKeywordVector(tempArgs);
	if (tempArgs.size() < 2)
	      	_ts.throwValidationError("needs at least one code and a URI", "error_page");
	std::string uri = tempArgs.back();
	try { _isValidURI(uri); }
	catch(const std::exception& e) { _ts.throwValidationError(e.what(), "error_page"); }
	tempArgs.pop_back();
	try
	{
		for (size_t i = 0; i < tempArgs.size(); ++i)
		{
			int errorCode = stringToNumber<int>(tempArgs[i]);
			server.errorPages[static_cast <t_status_code>(errorCode)] = uri;
		}
	}
	catch(const std::exception& e) { _ts.throwValidationError(e.what(), "error_page"); }
	
	_validate_ErrorPages(server.errorPages);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate the set of `error_page` status codes.
 *
 * Ensures each status code used in `server.errorPages` is within the
 * client/server error range [400,599]. Throws a validation error if any
 * code falls outside that range.
 *
 * @param errorPages Map of status code -> target URI to validate.
 */
void ConfigParser::_validate_ErrorPages(const std::map<t_status_code, std::string> &errorPages)
{
	std::map<t_status_code, std::string>::const_iterator it;
	for (it = errorPages.begin(); it != errorPages.end(); ++it)
	{
		int code = it->first;
		if (code < 400 || code > 599)
			_ts.throwValidationError("Invalid CODE", "error_page");
	}
}
//*	----------- *

///* CGI
/**
 * @brief Parse the `cgi` directive mapping a file extension to an executable.
 *
 * Expects two tokens: the extension (e.g. `.php`) and the executable path.
 * The extension is validated to begin with a dot and contain only
 * alphanumeric characters; the executable is validated as an existing
 * executable file. The mapping is stored in `location.cgi` and the
 * terminating semicolon is consumed.
 *
 * @param location LocationConfig to populate (modified in-place).
 */
void ConfigParser::_serverCgi(ServerConfig &server)
{
	std::string ext;
	std::string exec;
	_ts._extractSingleKeyword(ext);
	_ts._extractSingleKeyword(exec);
	_validate_Cgi(ext, exec);	
	server.cgi_default[ext] = exec;
	_ts._expect(TOKEN_SEMICOLON);
}
//*	----------- *

//*	----- LocationHandler functions ----- */

//* Location
//* Path
/**
 * @brief Parse a nested `location` block within a server block.
 *
 * Expects the `location` keyword has already been consumed by the caller.
 * This function reads the following path token, validates it for URI form
 * and uniqueness within the server using `locationsPathRecord`, consumes
 * the opening `{`, parses the inner location directives via
 * `_parseLocationBlock`, and finally appends the configured `LocationConfig`
 * to `server.locations`.
 *
 * @param server ServerConfig to which the parsed location will be added.
 * @param locationsPathRecord Set used to track and reject duplicate paths.
 */
void ConfigParser::_serverLocation(ServerConfig &server, std::set<std::string> &locationsPathRecord)
{
	t_token pathToken = _ts._expect(TOKEN_KEYWORD);

	LocationConfig newLocation;
	newLocation.path = pathToken.content;
	_validate_Path(newLocation.path, newLocation.CgiPass,locationsPathRecord);
	_ts._expect(TOKEN_LBRACE);

	_parseLocationBlock(newLocation);
	server.locations.push_back(newLocation);
}

/**
 * @brief Validate a `location` path token and ensure uniqueness.
 *
 * Checks that the path is a valid URI (via `_isValidURI`) and that it has
 * not already been used for another location within the same server. On
 * failure a validation error is thrown.
 *
 * @param path Path string to validate.
 * @param locationsPathRecord Set used to detect duplicate paths.
 */
void ConfigParser::_validate_Path(const std::string &path, bool &cgiPass, std::set<std::string> &locationsPathRecord)
{
	if (!path.empty() && path[0] == '*')
	{
		cgiPass = true;
		try { _isValidExtension(path.substr(1)); }
		catch(const std::exception& e) { _ts.throwValidationError(e.what(), "location path"); }
	}
	else
	{
		try { _isValidURI(path); }
		catch(const std::exception& e) { _ts.throwValidationError(e.what(), "location path"); }
	}

	if (locationsPathRecord.insert(path).second == false)
		_ts.throwValidationError("Duplicated", "location");
}
//*	----------- *

/**
 * @brief Parse the contents of a `location { ... }` block.
 *
 * Reads directives until the matching right brace `}`. Each directive
 * keyword is dispatched to the registered location handler table
 * (`_locationHandlers`). Unknown directives raise a syntax error. The
 * function consumes the closing `}` before returning.
 *
 * @param newLocation `LocationConfig` instance to populate (modified in-place).
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

///* Root
/**
 * @brief Parse the location-level `root` directive.
 *
 * Stores and validates the provided filesystem path for `location.root`.
 * Duplicate `root` directives within the same location are rejected.
 * Consumes the terminating semicolon.
 *
 * @param location LocationConfig to populate (modified in-place).
 */
void ConfigParser::_locationRoot(LocationConfig &location)
{
	if (!location.root.empty())
		_ts.throwValidationError("Duplicated", "root");
	_ts._extractSingleKeyword(location.root);
	_validate_Root(location.root);
	_ts._expect(TOKEN_SEMICOLON);
}
//*	----------- *

///* Index
/**
 * @brief Parse the location `index` directive (one or more filenames).
 *
 * Inserts the provided filenames into `location.index` and validates that
 * they are relative names (not starting or ending with `/`). Consumes the
 * terminating semicolon.
 *
 * @param location LocationConfig to populate (modified in-place).
 */
void ConfigParser::_locationIndex(LocationConfig &location)
{
	_ts._extractKeywordVector(location.index);
	_validate_Index(location.index);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate `index` filenames are relative (no leading/trailing '/').
 *
 * On invalid entries a syntax error is reported anchored to the current
 * token line.
 *
 * @param index Vector of index filename strings to validate.
 */
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

///* Auto Index
/**
 * @brief Parse `autoindex on|off` for the location.
 *
 * Accepts the literal `on` or `off` values. Duplicate directives are
 * rejected. The parsed boolean is stored in `location.autoindex` and the
 * terminating semicolon is consumed.
 *
 * @param location LocationConfig to populate (modified in-place).
 */
void ConfigParser::_locationAutoIndex(LocationConfig &location)
{
	if (location.autoindex != -1)
		_ts.throwValidationError("Duplicated", "autoindex");
	std::string autoindex;
	_ts._extractSingleKeyword(autoindex);
	if (autoindex != "on" && autoindex != "off")
		_ts.throwSyntaxError("invalid - must be 'on'/'off'", _ts._currentToken().line);
	location.autoindex = autoindex == "on" ?  true : false;
	_ts._expect(TOKEN_SEMICOLON);
}
//*	----------- *

///* Allow Methods
/**
 * @brief Parse the location `allow_methods` directive.
 *
 * Reads zero or more method keywords (GET/POST/DELETE/...) until a non-keyword
 * token is encountered, adding unique methods to `location.allowedMethods`.
 * The resulting vector is validated and the terminating semicolon consumed.
 *
 * @param location LocationConfig to populate (modified in-place).
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

/**
 * @brief Validate the parsed allowed HTTP methods for a location.
 *
 * Ensures the vector is not empty and does not contain `UNSUPPORTED_METHOD`.
 * On invalid inputs a validation error is thrown.
 *
 * @param allowedMethods Vector of parsed method enums to validate.
 */
void ConfigParser::_validate_AllowedMethods(const std::vector<t_method> &allowedMethods)
{
	if (allowedMethods.empty())
		_ts.throwValidationError("directive is empty", "allow_methods");
	for (size_t i = 0; i < allowedMethods.size(); ++i)    
	{
		if (allowedMethods[i] == UNSUPPORTED_METHOD)
			_ts.throwValidationError("Unsupported METHOD", "allow_methods");
	}
}
//*	----------- *

///* Return
/**
 * @brief Parse the location `return` directive (HTTP redirect).
 *
 * Expects a numeric status code followed by a URL. The code is validated to
 * be a supported redirect (300-308) and the URL validated via `_isValidURL`.
 * Duplicate `return` directives are rejected. The terminating semicolon is
 * consumed upon success.
 *
 * @param location LocationConfig to populate (modified in-place).
 */
void ConfigParser::_locationReturn(LocationConfig &location)
{
	if (location.returnCode != INVALID_CODE)
		_ts.throwValidationError("Duplicated", "return");
	std::string keyword;
	_ts._extractSingleKeyword(keyword);
	int	returnCode;
	try { returnCode = stringToNumber<int>(keyword); }
	catch(const std::exception& e) { _ts.throwValidationError(e.what(), "return"); }

	location.returnCode = HTTP::isValidReasonPhrase(returnCode);
	_ts._extractSingleKeyword(location.returnUrl);
	_validate_ReturnCode(location.returnCode, location.returnUrl);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate a `return` status code and its target URL.
 *
 * Ensures the status code exists and is within the supported redirect range
 * (300-308) and that the provided URL is syntactically valid.
 *
 * @param returnCode Parsed status code to validate.
 * @param returnURL Parsed target URL to validate.
 */
void ConfigParser::_validate_ReturnCode(const t_status_code returnCode, const std::string &returnURL)
{
	if (returnCode == INVALID_CODE)
		_ts.throwValidationError("Invalid CODE", "return");
	if (returnCode < 300 || returnCode > 308)
		_ts.throwValidationError("Unsupported CODE", "return");
	try { _isValidURL(returnURL); }
	catch (const std::exception &e) { _ts.throwValidationError(e.what(), "return"); }
}
//*	----------- *

///* CGI
/**
 * @brief Parse the `cgi` directive mapping a file extension to an executable.
 *
 * Expects two tokens: the extension (e.g. `.php`) and the executable path.
 * The extension is validated to begin with a dot and contain only
 * alphanumeric characters; the executable is validated as an existing
 * executable file. The mapping is stored in `location.cgi` and the
 * terminating semicolon is consumed.
 *
 * @param location LocationConfig to populate (modified in-place).
 */
void ConfigParser::_locationCgi(LocationConfig &location)
{
	std::string ext;
	std::string exec;
	if (_ts._previousToken().content == "cgi_pass")
	{
		ext = location.getPath().substr(1);
		location.CgiPass = true;
	}
	else
		_ts._extractSingleKeyword(ext);
	_ts._extractSingleKeyword(exec);
	_validate_Cgi(ext, exec);	
	location.cgi[ext] = exec;
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate a CGI extension and executable path.
 *
 * Ensures the extension begins with `.` and contains only alphanumeric
 * characters, and that the executer path refers to an existing executable
 * file with the required permissions.
 *
 * @param extension File extension token (e.g. `.py`, `.php`).
 * @param executer Filesystem path to the CGI executable.
 */
void ConfigParser::_validate_Cgi(std::string &extension, const std::string &executer)
{
	try {
		_isValidExtension(extension);
		extension.erase(0, 1);
		_isValidFile(executer, R_OK | X_OK); 
	}
	catch (const std::exception &e) { _ts.throwValidationError(e.what(), "cgi"); }
}
//*	----------- *

///* Upload Store
/**
 * @brief Parse the location `upload_store` directive.
 *
 * Stores a path used for file uploads and validates it is an existing
 * directory with write and execute permissions.
 *
 * @param location LocationConfig to populate (modified in-place).
 */
void ConfigParser::_locationUploadStore(LocationConfig &location)
{
	if (!location.uploadStore.empty())
		_ts.throwValidationError("Duplicated", "upload_store");
	_ts._extractSingleKeyword(location.uploadStore);
	_validate_UploadStore(location.uploadStore);
	_ts._expect(TOKEN_SEMICOLON);
}

/**
 * @brief Validate that the upload store path exists and is writable.
 *
 * Ensures the path is a directory with write and execute permissions.
 *
 * @param path Filesystem path to validate.
 */
void ConfigParser::_validate_UploadStore(std::string &path)
{
	if (path[path.length() - 1] != '/')
		path.append(1, '/');
	try { _isValidDirectory(path, W_OK | X_OK); }
	catch (const std::exception &e) { _ts.throwValidationError(e.what(), "upload_store"); }
}
//*	----------- *

//*	----- Helpers ----- */

void ConfigParser::_isValidExtension(const std::string &ext) const
{
	if (ext.length() < 2 || ext[0] != '.')
		throw std::runtime_error("Invalid extension");	
	for (size_t i = 1; i < ext.length(); ++i)
	{
		if (!std::iswalpha(ext[i]))
			throw std::runtime_error("Invalid extension");	
	}
}

/**
 * @brief Check whether a string is a valid URI path for `location`.
 *
 * Minimal check: non-empty and starts with '/'. More sophisticated
 * validation (percent-encoding, allowed characters) is intentionally
 * omitted to keep lexer/parser responsibilities focused and simple.
 *
 * @param uri URI string to validate.
 * @return true if it is a non-empty path starting with '/', false otherwise.
 */
void ConfigParser::_isValidURI(const std::string &uri) const
{
	if (uri.empty() || uri[0] != '/')
		throw std::runtime_error("Invalid URI - must start with '/'");
}

/**
 * @brief Validate a return/redirect URL.
 *
 * Accepts absolute `http://` or `https://` URLs with at least one
 * character after the scheme, or a server-local path beginning with `/`.
 * This is a syntactic check and does not verify network reachability.
 *
 * @param url URL string to validate.
 * @return true if syntactically valid, false otherwise.
 */
void ConfigParser::_isValidURL(const std::string &url) const
{
	if (url.empty())
		throw std::runtime_error("Empty URL");
	if (url.length() >= 7 && url.compare(0, 7, "http://") == 0)
	{
		if (url.length() == 7)
			throw std::runtime_error("Invalid URL - empty 'http://'");
	}
	else if (url.length() >= 8 && url.compare(0, 8, "https://") == 0)
	{
		if (url.length() == 8)
			throw std::runtime_error("Invalid URL - empty 'https://'");
	}
	else if (url[0] != '/')
		throw std::runtime_error("Invalid URL");
}

/**
 * @brief Wrapper around `access(2)` to check filesystem permissions.
 *
 * Returns true when `access(path, flags)` indicates the current process
 * has the requested permissions; false otherwise.
 *
 * @param path Filesystem path to check.
 * @param flags Permission flags as accepted by `access(2)` (e.g. R_OK).
 * @return true if accessible with given flags, false otherwise.
 */
void ConfigParser::_isValidAccess(const std::string &path, const int flags) const
{
	if (access(path.c_str(), flags) != 0)
		throw std::runtime_error("Couldn't access given path");
}

/**
 * @brief Validate that a path refers to a regular file with the requested
 * access permissions.
 *
 * Uses `_isValidAccess` and `stat(2)` to ensure the path exists, is not a
 * directory, and the current process has the requested permissions.
 *
 * @param path Filesystem path to validate.
 * @param flags Permission flags to check with `access(2)`.
 * @return true if the path is a file and accessible, false otherwise.
 */
void ConfigParser::_isValidFile(const std::string &path, const int flags) const
{
	_isValidAccess(path, flags);
	struct stat path_stat;
	if (stat(path.c_str(), &path_stat) != 0)
		throw std::runtime_error("Couldn't access given path");
	if (S_ISDIR(path_stat.st_mode))
		throw std::runtime_error("Given path is not a File");
}

/**
 * @brief Validate that a path refers to a directory with the requested
 * access permissions.
 *
 * Uses `_isValidAccess` and `stat(2)` to ensure the path exists, is a
 * directory, and the current process has the requested permissions.
 *
 * @param path Directory path to validate.
 * @param flags Permission flags to check with `access(2)`.
 * @return true if the path is a directory and accessible, false otherwise.
 */
void ConfigParser::_isValidDirectory(const std::string &path, const int flags) const
{
	_isValidAccess(path, flags);
	struct stat path_stat;
	if (stat(path.c_str(), &path_stat) != 0)
		throw std::runtime_error("Couldn't access given path");
	if (!S_ISDIR(path_stat.st_mode))
		throw std::runtime_error("Given path is not a Directory");
}


/*

	! CHECK ERROR MESSAGES -> NOTE TO ADD LINE NUMBER / *COLLUMN?*

	! CHECK FILE NAME EXTENSION -> LEXER

	! CHECK what auotindex do, must have index filled if autoindex on?

	! ERROR LINE SERVER COLLISION NAME AND LISTEN

	! fix -> servername collision -> void ConfigParser::_validate_ServerCollision(const std::vector<ServerConfig> &servers)

*/
