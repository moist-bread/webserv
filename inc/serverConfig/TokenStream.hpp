#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>

#include "token.h" // t_token

// =====>┊( CONFIG )┊

/**
 * @class TokenStream
 * @brief Lightweight cursor over a token vector used by the parser.
 *
 * `TokenStream` provides a thin, read-only view over a sequence of tokens
 * produced by the `Lexer`. It maintains a cursor index used by the parser to
 * inspect and consume tokens in order. The class also centralizes syntax and
 * validation error formatting so errors include file line information and a
 * consistent message style. The parser/consumer owns the token vector; this
 * class does not perform tokenization or file I/O.
 */
class TokenStream
{
	public:
		TokenStream(const std::vector<t_token>&);
		~TokenStream(void);

		std::string _tokenTypeToString(e_token_type type) const;

		void _advanceToken(void);
		const t_token& _currentToken(void) const;
		const t_token& _previousToken(void) const;

		t_token _expect(e_token_type);
		void _extractSingleKeyword(std::string&);
		void _extractKeywordVector(std::vector<std::string>&);

		void throwSyntaxError(const std::string &message, size_t customLine = 0) const;
		void throwValidationError(const std::string &message, const std::string &directive) const;

	private:
		size_t	_cursor;
		const std::vector<t_token> &_tokens;
};

std::ostream &operator<<(std::ostream &out, TokenStream const &src);
