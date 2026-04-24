#pragma once

// ==┊ needed libs by class
# include "token.h" 
# include <iostream>
# include <vector>

// =====>┊( LEXER )┊

/**
 * @class Lexer
 * @brief Static utility class for tokenizing configuration files
 * 
 * The Lexer class provides static methods to parse configuration files (nginx-style)
 * into a sequence of tokens. It handles keywords, symbols ({, }, ;), and comments (#).
 * This is a utility class with only static methods and cannot be instantiated.
 * 
 */
class Lexer
{
	public:
		static std::vector<t_token> tokenizeFile(const std::string &filePath);

	private:
		Lexer(void);
		Lexer(Lexer const &src);
		~Lexer(void);
		Lexer &operator=(Lexer const &src);
		
		static void _tokenizeLine(const std::string &line,  size_t lineNumber, std::vector<t_token> &outTokens);
		static void _skipSpaces(const std::string &line, size_t &cursor);
		static t_token _getSymbol(const std::string &line, size_t &cursor, size_t lineNumber);
		static t_token _getKeyword(const std::string &line, size_t &cursor, size_t lineNumber);
		static void _pushEOF(size_t lineNumber, std::vector<t_token> &outTokens);
};

std::ostream &operator<<(std::ostream &out, const std::vector<t_token> &src);
