#pragma once

# include <iostream>

/**
 * @enum e_token_type
 * @brief Types of tokens recognized by the lexer
 * 
 * Defines all possible token types that can appear in a configuration file.
 */
enum e_token_type
{
	TOKEN_KEYWORD,		///< Configuration directives, identifiers, values (e.g., "server", "8080")
	TOKEN_LBRACE,		///< Left brace '{' - starts a configuration block
	TOKEN_RBRACE,		///< Right brace '}' - ends a configuration block
	TOKEN_SEMICOLON,	///< Semicolon ';' - terminates a configuration directive
	TOKEN_EOF			///< End of file marker - signals end of tokenization
};

/**
 * @struct s_token
 * @brief Represents a single token from the configuration file
 * 
 * Contains all information about a token: its type, textual content,
 * and position in the source file.
 */
typedef struct	s_token
{
	e_token_type	type;		///< Type of the token (keyword, symbol, EOF)
	std::string		content;	///< Textual content (e.g., "listen", "8080", "{")
	size_t			line;		///< Line number where token appears (1-indexed)
} 				t_token;

