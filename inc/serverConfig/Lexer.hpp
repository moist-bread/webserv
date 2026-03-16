#pragma once

// ==┊ needed libs by class
# include <iostream>
# include <vector>
# include "../ansi_color_codes.h"
# include "token.h" 

// =====>┊( CLASS )┊

/**
 * @class Lexer
 * @brief Static utility class for tokenizing configuration files
 * 
 * The Lexer class provides static methods to parse configuration files (nginx-style)
 * into a sequence of tokens. It handles keywords, symbols ({, }, ;), and comments (#).
 * This is a utility class with only static methods and cannot be instantiated.
 * 
 * @note Thread-safe (no shared state)
 * @note C++98 compliant
 */
class Lexer
{
	public:
		static std::vector<t_token> tokenizeFile(const std::string &filePath);

	private:
		// Private constructor - class cannot be instantiated
		Lexer(void);
		Lexer(Lexer const &source);
		~Lexer(void);
		Lexer &operator=(Lexer const &source);
		
		static void _tokenizeLine(const std::string &line,  size_t lineNumber, std::vector<t_token> &outTokens);
		static void _skipSpaces(const std::string &line, size_t &cursor);
		static t_token _getSymbol(const std::string &line, size_t &cursor, size_t lineNumber);
		static t_token _getKeyword(const std::string &line, size_t &cursor, size_t lineNumber);
		static void _pushEOF(size_t lineNumber, std::vector<t_token> &outTokens);
};

std::ostream &operator<<(std::ostream &out, const std::vector<t_token> &source);
