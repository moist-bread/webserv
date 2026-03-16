#pragma once

// ==┊ needed libs by class
#include <iostream>

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

		//	Helpers
		t_token _expect(e_token_type expected_type);
		const t_token &_currentToken(void) const;
		void _advanceToken(void); 

		size_t	_cursor;
		const std::vector<t_token> &_tokens;
};

std::ostream &operator<<(std::ostream &out, Parser const &source);
