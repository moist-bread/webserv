#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>
#include <map>
#include <stdlib.h>

#include "../ansi_color_codes.h"
#include "token.h"

class ServerConfig;
class LocationConfig;

/**
 * @class Parser
 * @brief Parses a token stream into server configuration objects.
 *
 * The parser consumes tokens produced by the lexer and builds
 * `ServerConfig` and nested `LocationConfig` objects.
 */

class Parser
{
	public:
		Parser(const std::vector<t_token> &tokens);	// main constructor
		~Parser(void);								// destructor

		void parse(std::vector<ServerConfig> &servers);

	private:
		Parser(void); 					// default constructor
		Parser(Parser const &source);	// copy constructor
		Parser &operator=(Parser const &source); // copy assignment operator overload

		ServerConfig _parseServerBlock(void);
		LocationConfig _parseLocationBlock(const std::string &path);

		typedef void (Parser::*ServerHandler)(ServerConfig &);		//	custom type called 'ServerHandler' -> pointer to a Parser method that takes a ServerConfig reference
		typedef void (Parser::*LocationHandler)(LocationConfig &);	//	custom type called 'LocationHandler' -> pointer to a Parser method that takes a LocationConfig reference

		std::map<std::string, ServerHandler> _serverHandlers;		//	map links the string (e.g., "listen") to the function pointer
		std::map<std::string, LocationHandler> _locationHandlers;	//	map links the string (e.g., "root") to the function pointer

		//	ServerHandler functions
		void _serverListen(ServerConfig &server);
		void _serverName(ServerConfig &server);
		void _serverMaxBodySize(ServerConfig &server);
		void _serverErrorPage(ServerConfig &server);
		void _serverLocation(ServerConfig &server);

		//	LocationHandler functions
		void _locationRoot(LocationConfig &location);
		void _locationIndex(LocationConfig &location);
		void _locationAutoIndex(LocationConfig &location);
		void _locationAllowMethods(LocationConfig &location);
		void _locationReturn(LocationConfig &location);
		void _locationCgi(LocationConfig &location);
		void _locationUploadStore(LocationConfig &location);


		//	ServerHandler Validation functions
		static void _validate_Listen(const std::string &host, const int port);
		static void _validate_ServerNames(const std::vector<std::string> &serverNames);
		static void _validate_MaxBodySize(const size_t clientMaxBodySize);
		static void _validate_ErrorPages(const std::map<t_status_code, std::string> &errorPages);
		static void _validate_NameServer_Collision(const ServerConfig &server, const std::vector<std::string> &claimedNames);

		//	LocationHandler Validation functions
		static void _validate_AllowedMethods(const std::vector<t_method> &allowedMethods);
	
		static void _add_to_ClaimedNames(const ServerConfig &server, std::vector<std::string> &dest);
		

		//	Helpers
		t_token _expect(e_token_type expected_type);
		const t_token &_currentToken(void) const;
		void _advanceToken(void); 
		void _extractSingleKeyword(std::string &destination);
		void _extractKeywordVector(std::vector<std::string> &destination);

		size_t	_cursor;
		const std::vector<t_token> &_tokens;

};
