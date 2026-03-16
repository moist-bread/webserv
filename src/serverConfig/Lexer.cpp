# include "../../inc/serverConfig/Lexer.hpp"
# include <fstream>
# include <string>
# include <stdexcept>

/**
 * @brief Private default constructor (unused)
 * 
 * This constructor is private to prevent instantiation of the Lexer class.
 * Since all methods are static, creating instances is unnecessary and prevented.
 */
Lexer::Lexer(void)
{
	std::cout << GRN "the Lexer ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Lexer::Lexer(Lexer const &source)
{
	(void)source;
	std::cout << GRN "the Lexer ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Lexer::~Lexer(void)
{
	std::cout << GRN "the Lexer ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Lexer &Lexer::operator=(Lexer const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	(void)source;
	return (*this);
}

/**
 * @brief Tokenizes a configuration file into tokens
 * 
 * Opens and reads the specified file line by line, parsing each line into tokens.
 * The function handles:
 * - Keywords (alphanumeric sequences)
 * - Symbols: { } ;
 * - Comments (# to end of line)
 * - Whitespace (skipped)
 * 
 * @param filePath Path to the configuration file
 * @return Vector containing all tokens from the file, with EOF token at the end
 * 
 * @throws std::runtime_error If file cannot be opened
 * 
 * @note Automatically adds EOF token at the end
 * @note Comments are not included in output tokens
 */
std::vector<t_token> Lexer::tokenizeFile(const std::string &filePath)
{
	std::vector<t_token> outTokens;
	std::ifstream file(filePath.c_str());
	if (!file.is_open())
		throw std::runtime_error("Failed to open file: " + filePath);

	outTokens.clear();
	std::string line;
	size_t currentLine = 1;
	while (std::getline(file, line))	
	{
		_tokenizeLine(line, currentLine, outTokens);
		currentLine++;
	}
	file.close();
	_pushEOF(currentLine, outTokens);
	return (outTokens);
}

/**
 * @brief Tokenizes a single line of text
 * 
 * Iterates through the line character by character, identifying and extracting tokens.
 * Handles whitespace skipping, comment detection, symbol recognition, and keyword extraction.
 * 
 * @param line The line of text to tokenize
 * @param lineNumber Line number for error reporting and token tracking
 * @param outTokens Vector where extracted tokens are appended
 * 
 * @note Stops processing at '#' (comment start)
 * @note Does not add newline tokens
 */
void Lexer::_tokenizeLine(const std::string &line, size_t lineNumber, std::vector<t_token> &outTokens)
{
	size_t cursor = 0;
	while (cursor < line.length())
	{
		char c = line[cursor];
		if (c == '#') 
			break;
		else if (std::isspace(c))
			_skipSpaces(line, cursor);
		else if (c == '{' || c == '}' || c == ';')
			outTokens.push_back(_getSymbol(line, cursor, lineNumber));
		else
			outTokens.push_back(_getKeyword(line, cursor, lineNumber));
	}
}

/**
 * @brief Advances cursor past all whitespace characters
 * 
 * Moves the cursor forward through consecutive whitespace characters (spaces, tabs, etc.)
 * until a non-whitespace character or end of line is reached.
 * 
 * @param line The line being parsed
 * @param cursor Current position (modified to skip whitespace)
 * 
 * @note Uses std::isspace() to detect whitespace
 * @note Includes bounds checking to prevent out-of-range access
 */
void Lexer::_skipSpaces(const std::string &line, size_t &cursor)
{
	while (cursor < line.length() && std::isspace(line[cursor]))
		cursor++;
}

/**
 * @brief Extracts a symbol token at the current cursor position
 * 
 * Creates a token for single-character symbols used in configuration syntax.
 * Recognized symbols:
 * - '{' : TOKEN_LBRACE (block open)
 * - '}' : TOKEN_RBRACE (block close)
 * - ';' : TOKEN_SEMICOLON (statement end)
 * 
 * @param line The line being parsed
 * @param cursor Current position in line (incremented after extraction)
 * @param lineNumber Line number for the token
 * @return Token representing the symbol at cursor position
 */
t_token Lexer::_getSymbol(const std::string &line, size_t &cursor, size_t lineNumber)
{
	t_token token;
	token.content = line[cursor];
	token.line = lineNumber;

	if (line[cursor] == '{')
		token.type = TOKEN_LBRACE;
	else if (line[cursor] == '}')
		token.type = TOKEN_RBRACE;
	else if (line[cursor] == ';')
		token.type = TOKEN_SEMICOLON;
	cursor++;
	return (token);
}

/**
 * @brief Extracts a keyword token starting at the current cursor position
 * 
 * Reads consecutive characters that form a keyword (directive name, value, etc.).
 * Stops when encountering:
 * - Whitespace
 * - Symbols: { } ;
 * - Comment start: #
 * - End of line
 * 
 * @param line The line being parsed
 * @param cursor Current position in line (updated to character after keyword)
 * @param lineNumber Line number for the token
 * @return Token containing the keyword string
 * 
 * @note Includes bounds checking to prevent buffer overflow
 */
t_token Lexer::_getKeyword(const std::string &line, size_t &cursor, size_t lineNumber)
{
	t_token token;
	token.type = TOKEN_KEYWORD;
	token.line = lineNumber;

	size_t needle = cursor;
	while (needle < line.length() && line[needle] != '{' && line[needle] != '}'
			&& line[needle] != ';' && line[needle] != '#' && !std::isspace(line[needle]))
		needle++;
	token.content = line.substr(cursor, needle - cursor);
	cursor = needle;
	return (token);
}

/**
 * @brief Appends an end-of-file token to the token vector
 * 
 * Creates a TOKEN_EOF token and adds it to the end of the token vector.
 * This marker indicates successful completion of tokenization.
 * 
 * @param lineNumber Line number where EOF occurs (typically last line number + 1)
 * @param outTokens Vector where the EOF token is appended
 */
void Lexer::_pushEOF(size_t lineNumber, std::vector<t_token> &outTokens)
{
	t_token token;
	token.type = TOKEN_EOF;
	token.line = lineNumber;
	outTokens.push_back(token);
}

//
//	'<<' overload operator -> print vector
//

/**
 * @brief Converts token type enum to human-readable string
 * 
 * Helper function for debugging and display purposes.
 * 
 * @param tokenType The token type to convert
 * @return String representation of the token type
 */
static std::string getTokenTypeName(e_token_type tokenType)
{
	switch (tokenType)
	{
		case TOKEN_KEYWORD: return "KEYWORD";
		case TOKEN_LBRACE: return "LBRACE";
		case TOKEN_RBRACE: return "RBRACE";
		case TOKEN_SEMICOLON: return "SEMICOLON";
		case TOKEN_EOF: return "EOF";
		default: return "UNKNOWN";
	}
}

/**
 * @brief Outputs token vector to stream in human-readable format
 * 
 * Prints each token on a separate line with format:
 * "Line: <line_number> [<TYPE>] '<content>'"
 * 
 * @param out Output stream
 * @param source Vector of tokens to display
 * @return Reference to output stream for chaining
 * 
 * @note Useful for debugging and visualization of tokenization results
 */
std::ostream &operator<<(std::ostream &out, const std::vector<t_token> &source)
{
	std::vector<t_token>::const_iterator it;
	for (it = source.begin(); it != source.end(); it++)
		out << "Line: " << it->line << " [" << getTokenTypeName(it->type) << "] '" << it->content << "'\n";
	return (out);
}

