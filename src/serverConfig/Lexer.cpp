# include "../../inc/serverConfig/Lexer.hpp"

# include <fstream> // std::ifstream
# include <string> // std::string
# include <stdexcept> // std::runtime_error
# include <unistd.h> // access
# include <sys/stat.h> // stat

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

 //Linha 72 Lexer.cpp no ficheiro tokenizeFile esta dar segfault ao mandar uma pasta ./webserv config_files

std::vector<t_token> Lexer::tokenizeFile(const std::string &filePath)
{
	std::vector<t_token> outTokens;
	parsePathFile(filePath, R_OK);
	std::ifstream file(filePath.c_str());
	if (!file.is_open())
		throw std::runtime_error("Failed to open file: '" + filePath + "'");

	outTokens.clear();
	std::string line;
	size_t currentLine = 1;
	while (std::getline(file, line))	
	{
		_tokenizeLine(line, currentLine, outTokens);
		currentLine++;
	}
	file.close();

	if (outTokens.empty())
		throw std::runtime_error("Empty file");
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
 * @return Token representing the symbol at cursor position. The token's
 *         `content` is a single character, `type` set accordingly and
 *         `collumn` set to the 1-based column where the symbol started.
 */
t_token Lexer::_getSymbol(const std::string &line, size_t &cursor, size_t lineNumber)
{
	t_token token;
	token.content = line[cursor];
	token.line = lineNumber;
	token.collumn = cursor + 1;

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
 * @return Token containing the keyword string (type `TOKEN_KEYWORD`, `content` set).
 *
 * @note The returned token's `collumn` is set to the 1-based starting column.
 */
t_token Lexer::_getKeyword(const std::string &line, size_t &cursor, size_t lineNumber)
{
	t_token token;
	token.type = TOKEN_KEYWORD;
	token.line = lineNumber;
	token.collumn = cursor + 1;

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
	token.collumn = 0;
	outTokens.push_back(token);
}

//* ---- Helpers ----

/**
 * @brief Wrapper around `access(2)` to check filesystem permissions.
 *
 * Returns true when `access(path, flags)` indicates the current process
 * has the requested permissions; false otherwise.
 *
 * @param path Filesystem path to check.
 * @param flags Permission flags as accepted by `access(2)` (e.g. R_OK).
 * @return true if accessible with given flags, false otherwise.
 */
static void isValidAccess(const std::string &path, const int flags)
{
	if (access(path.c_str(), flags) != 0)
		throw std::runtime_error("Couldn't access: '" + path + "'");
}

/**
 * @brief Validate that a path refers to a regular file with the requested
 * access permissions.
 *
 * Uses `_isValidAccess` and `stat(2)` to ensure the path exists, is not a
 * directory, and the current process has the requested permissions.
 *
 * @param path Filesystem path to validate.
 * @param flags Permission flags to check with `access(2)`.
 * @return true if the path is a file and accessible, false otherwise.
 */
static void isValidFile(const std::string &path, const int flags)
{
	isValidAccess(path, flags);
	struct stat path_stat;
	if (stat(path.c_str(), &path_stat) != 0)
		throw std::runtime_error("Couldn't access: '" + path + "'");
	if (S_ISDIR(path_stat.st_mode))
		throw std::runtime_error("'" + path + "' is not a File");
}

void Lexer::parsePathFile(const std::string &path, const int flag)
{
	size_t extension = path.rfind(".conf");
	if (extension == std::string::npos || extension != path.length() - 5)
		throw std::runtime_error("Bad extension");
	isValidFile(path, flag);
}


/**
 * @brief Internal helper to convert token enum to readable name.
 *
 * This function is a small debugging aid used by the `operator<<` overload
 * to produce human-readable token dumps. It is intentionally `static` to
 * restrict linkage to this translation unit.
 *
 * @param tokenType Token enum value.
 * @return Short name string for the token type.
 */
static std::string getTokenTypeName(e_token_type tokenType)
{
	switch (tokenType)
	{
		case TOKEN_KEYWORD: 	return "KEYWORD";
		case TOKEN_LBRACE: 		return "LBRACE";
		case TOKEN_RBRACE: 		return "RBRACE";
		case TOKEN_SEMICOLON: 	return "SEMICOLON";
		case TOKEN_EOF: 		return "EOF";
		default: 				return "UNKNOWN";
	}
}
//*	----- ------ ----- //

/**
 * @brief Outputs token vector to stream in human-readable format
 * 
 * Prints each token on a separate line with format:
 * "Line: <line_number> [<TYPE>] '<content>'"
 * 
 * @param out Output stream
 * @param src Vector of tokens to display
 * @return Reference to output stream for chaining
 * 
 * @note Useful for debugging and visualization of tokenization results
 */
std::ostream &operator<<(std::ostream &out, const std::vector<t_token> &src)
{
	std::vector<t_token>::const_iterator it;
	for (it = src.begin(); it != src.end(); it++)
		out << "Line: " << it->line << " [" << getTokenTypeName(it->type) << "] '" << it->content << "'\n";
	return (out);
}

