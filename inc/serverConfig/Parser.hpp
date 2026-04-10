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

// =====>┊( CLASS )┊

class Parser
{
	public:
		Parser(const std::vector<t_token> &tokens);	// main constructor
		~Parser(void);								// destructor

		void parse(std::vector<ServerConfig> &servers);

	private:
		// Parser(void); 				// default constructor
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

		//	Helpers
		t_token _expect(e_token_type expected_type);
		const t_token &_currentToken(void) const;
		void _advanceToken(void); 
		void _extractSingleKeyword(std::string &destination);
		void _extractKeywordVector(std::vector<std::string> &destination);

		size_t	_cursor;
		const std::vector<t_token> &_tokens;

};

std::ostream &operator<<(std::ostream &out, Parser const &source);
