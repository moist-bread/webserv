#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stdlib.h>

#include "../ansi_color_codes.h"
#include "token.h"
#include "../http/HTTP.hpp"
#include "../../inc/serverConfig/TokenStream.hpp"

struct ServerConfig;
struct LocationConfig;
class TokenStream;

/**
 * @class ConfigParser
 * @brief Convert a token sequence into runtime server configuration objects.
 *
 * `ConfigParser` is responsible for validating and transforming the flat
 * token stream produced by the `Lexer` into a structured vector of
 * `ServerConfig` objects (each containing nested `LocationConfig` entries).
 * The class centralizes syntactic and semantic validation, reporting errors
 * via the attached `TokenStream` helpers so messages include line anchors
 * and consistent formatting. The parser does not perform I/O; it operates
 * entirely on the provided token vector.
 *
 * Usage: construct with the token vector and call `parse()` to obtain the
 * resulting vector of `ServerConfig` instances. The parser assumes the token
 * stream ends with `TOKEN_EOF`.
 */
class ConfigParser
{
public:
	ConfigParser(const std::vector<t_token> &tokens); // main constructor
	~ConfigParser(void);							  // destructor

	std::vector<ServerConfig> parse(void);

private:
	ConfigParser(void);									 // default constructor
	ConfigParser(ConfigParser const &source);			 // copy constructor
	ConfigParser &operator=(ConfigParser const &source); // copy assignment operator overload

	void _parseServerBlock(ServerConfig &newServer);
	void _parseLocationBlock(LocationConfig &newLocation);

	typedef void (ConfigParser::*ServerHandler)(ServerConfig &);	 //	custom type called 'ServerHandler' -> pointer to a Parser method that takes a ServerConfig reference
	typedef void (ConfigParser::*LocationHandler)(LocationConfig &); //	custom type called 'LocationHandler' -> pointer to a Parser method that takes a LocationConfig reference

	std::map<std::string, ServerHandler> _serverHandlers;	  //	map links the string (e.g., "listen") to the function pointer
	std::map<std::string, LocationHandler> _locationHandlers; //	map links the string (e.g., "root") to the function pointer

	//	ServerHandler functions
	void _serverListen(ServerConfig &server);
	void _serverName(ServerConfig &server);
	void _serverRoot(ServerConfig &server);
	void _serverMaxBodySize(ServerConfig &server);
	void _serverErrorPage(ServerConfig &server);
	void _serverCgi(ServerConfig &server);
	void _serverLocation(ServerConfig &server, std::set<std::string> &locationsPathRecord);
	void _validate_ServerCollision(const std::vector<ServerConfig> &servers);
	void _finalizeServer(ServerConfig &server);
	void _finalizeLocation(ServerConfig &server);
	void _finalizeLocationCgiPass(LocationConfig &location, ServerConfig &server);
	void _finalizeLocationReturn(LocationConfig &location);

	//	LocationHandler functions
	void _locationRoot(LocationConfig &location);
	void _locationIndex(LocationConfig &location);
	void _locationAutoIndex(LocationConfig &location);
	void _locationAllowMethods(LocationConfig &location);
	void _locationReturn(LocationConfig &location);
	void _locationCgi(LocationConfig &location);
	void _locationUploadStore(LocationConfig &location);

	//	ServerHandler Validation functions
	void _validate_Listen(const std::string &host, const int port);
	void _validate_ServerNames(const std::vector<std::string> &serverNames);
	void _validate_Root(std::string &root);
	void _validate_MaxBodySize(const size_t clientMaxBodySize);
	void _validate_ErrorPages(const std::map<t_status_code, std::string> &errorPages);
	void _validate_ServerNamesCollision(const ServerConfig &server_A, const ServerConfig &server_B);

	//	LocationHandler Validation functions
	void _validate_Path(const std::string &path, bool &cgiPass, std::set<std::string> &locationsPathRecord);
	void _validate_Index(const std::vector<std::string> &index);
	void _validate_AllowedMethods(const std::vector<t_method> &allowedMethods);
	void _validate_ReturnCode(const t_status_code returnCode, const std::string &returnURL);
	void _validate_Cgi(std::string &extension, const std::string &executer);
	void _validate_UploadStore(std::string &path);

	//	Validate Helpers
	void _isValidURI(const std::string &uri) const;
	void _isValidURL(const std::string &url) const;
	void _isValidExtension(const std::string &ext) const;
	void _isValidAccess(const std::string &path, const int flags) const;
	void _isValidFile(const std::string &path, const int flags) const;
	void _isValidDirectory(const std::string &path, const int flags) const;

	TokenStream _ts;
};
