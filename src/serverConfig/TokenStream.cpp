#include "../../inc/serverConfig/TokenStream.hpp"
#include <sstream>

TokenStream::TokenStream(const std::vector<t_token> &tokens) : _cursor(0), _tokens(tokens) {}

TokenStream::TokenStream(const TokenStream &obj) : _cursor(obj._cursor), _tokens(obj._tokens) {}

TokenStream::~TokenStream(void) {}

std::string TokenStream::_tokenTypeToString(e_token_type type) const
{
	switch (type) {
		case TOKEN_SEMICOLON:	return ";";
		case TOKEN_LBRACE:		return "{";
		case TOKEN_RBRACE:		return "}";
		case TOKEN_KEYWORD:		return "keyword";
		default:				return "unknown token";
	}
}

/**
 * @brief Consume token if it matches expected type.
 * @param expected_type Expected token type.
 * @return Consumed token.
 * @throw std::runtime_error On mismatch or unexpected EOF.
 */
t_token TokenStream::_expect(e_token_type expected_type)
{
	if (this->_cursor >= _tokens.size())
		throwSyntaxError("Unexpected end of file. Missing '" + _tokenTypeToString(expected_type) + "'");

	t_token current = _currentToken();
	if (current.type != expected_type)
	{
		size_t errorLine = current.line;
        if ((expected_type == TOKEN_SEMICOLON || expected_type == TOKEN_LBRACE) && _cursor > 0)
            errorLine = _tokens[_cursor - 1].line;

        throwSyntaxError("Unexpected token '" + current.content + "' - Expecting '" + 
 				_tokenTypeToString(expected_type) + "'", errorLine);
	}
	_advanceToken();
	return (current);
}

/**
 * @brief Get current token without consuming it.
 * @return Reference to current token.
 * @throw std::runtime_error If cursor is out of range.
 */
const t_token &TokenStream::_currentToken(void) const
{
	if (this->_cursor >= this->_tokens.size())
		throw std::runtime_error("TokenStream error: Unexpected end of tokens");
	return (this->_tokens[this->_cursor]);
}

/**
 * @brief Advance Configparser cursor by one token when possible.
 */
void TokenStream::_advanceToken(void)
{
	if (this->_cursor < this->_tokens.size())
		this->_cursor++;
}

/**
 * @brief Extract one keyword token content into destination.
 * @param destination Output string.
 */
void TokenStream::_extractSingleKeyword(std::string &destination)
{
	t_token token = _expect(TOKEN_KEYWORD);
	destination = token.content;
}

/**
 * @brief Extract all consecutive keyword token contents.
 * @param destination Output vector to append keyword contents into.
 */
void TokenStream::_extractKeywordVector(std::vector<std::string> &destination)
{
	while (_currentToken().type == TOKEN_KEYWORD)
	{
		std::string newName = _currentToken().content;
        
        if (std::find(destination.begin(), destination.end(), newName) == destination.end())
			destination.push_back(newName);
		_advanceToken();
	}
}

/**
 * @brief Throws a formatted error using the CURRENT cursor's line number.
 */
void TokenStream::throwSyntaxError(const std::string &message, size_t customLine) const
{
	size_t line = customLine;
    
	if (line == 0)
		line = (_cursor < _tokens.size()) ? _tokens[_cursor].line : (_tokens.empty() ? 1 : _tokens.back().line);
    std::stringstream ss;
    ss << "Syntax error {l." << line << "} : " << message;
    throw std::runtime_error(ss.str());
}

/**
 * @brief Throws a formatted error using the PREVIOUS cursor's line number.
 */
void TokenStream::throwValidationError(const std::string &message, const std::string &directive) const
{
    size_t line = (_cursor > 0 && !_tokens.empty()) ? _tokens[_cursor - 1].line : 1;

    std::stringstream ss;
    ss << "Config error {l." << line << "} ";
	if (!directive.empty())
        ss << "in '" << directive << "'";
    ss << " : " << message;
    throw std::runtime_error(ss.str());
}
