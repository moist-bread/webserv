#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <vector>

#include "token.h"

// =====>┊( CONFIG )┊

class TokenStream
{
	public:
		TokenStream(const std::vector<t_token>&);
		TokenStream(const TokenStream&);
		~TokenStream(void);

		std::string _tokenTypeToString(e_token_type type) const;

		void _advanceToken(void);
		const t_token& _currentToken(void) const;

		t_token _expect(e_token_type);
		void _extractSingleKeyword(std::string&);
		void _extractKeywordVector(std::vector<std::string>&);

		void throwSyntaxError(const std::string &message, size_t customLine = 0) const;
		void throwValidationError(const std::string &message, const std::string &directive) const;

	private:
		TokenStream(void);
		TokenStream &operator=(TokenStream const &);

		size_t	_cursor;
		const std::vector<t_token> &_tokens;
};

std::ostream &operator<<(std::ostream &out, TokenStream const &src);
