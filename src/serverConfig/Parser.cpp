#include "../../inc/serverConfig/Parser.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include <sstream>
#include <stdexcept>

Parser::Parser(const std::vector<t_token> &tokens) : _cursor(0), _tokens(tokens)
{
	std::cout << GRN "the Parser ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

// Parser::Parser(void) : _tokens()
// {
// 	std::cout << GRN "the Parser ";
// 	std::cout << UCYN "has been created" DEF << std::endl;
// }

Parser::Parser(Parser const &source) : _cursor(0), _tokens(source._tokens)
{
	*this = source;
	std::cout << GRN "the Parser ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Parser::~Parser(void)
{
	std::cout << GRN "the Parser ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Parser &Parser::operator=(Parser const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, Parser const &source)
{
	(void)source;
	out << BLU "Parser";
	out << DEF << std::endl;
	return (out);
}

void Parser::parse(std::vector<ServerConfig> &servers)
{
	while (_currentToken().type != TOKEN_EOF)
	{
		if (_currentToken().type == TOKEN_KEYWORD && _currentToken().content == "server")
		{
			_advanceToken();
			ServerConfig newServer = _parseServerBlock();
			servers.push_back(newServer);
		}
		else
			throw std::runtime_error("Syntax error at line " /* + line number */ ": Expected 'server' block");
	}
}

ServerConfig Parser::_parseServerBlock(void)
{
	ServerConfig newServer;
	_expect(TOKEN_LBRACE);
	while (_currentToken().type != TOKEN_RBRACE)
	{
		t_token directiveToken = _expect(TOKEN_KEYWORD);
		if (directiveToken.content == "listen")
		{
			t_token listenToken = _expect(TOKEN_KEYWORD);
			size_t colonPos = listenToken.content.find(':');
			if (colonPos != std::string::npos)
			{
				newServer.host = listenToken.content.substr(0, colonPos);
				newServer.port = std::atoi(listenToken.content.substr(colonPos + 1).c_str());
			}
			else
			{
				newServer.host = "0.0.0.0";
				newServer.port = std::atoi(listenToken.content.c_str());
			}

			newServer.port = std::atoi(listenToken.content.c_str());
			_expect(TOKEN_SEMICOLON);
		}
		else if (directiveToken.content == "server_name")
		{
			while (_currentToken().type == TOKEN_KEYWORD)
			{
				newServer.serverNames.push_back(_currentToken().content);
				_advanceToken();
			}
			_expect(TOKEN_SEMICOLON);
			if (newServer.serverNames.empty())
				throw std::runtime_error("Syntax error: server_name directive is empty");
		}
		else if (directiveToken.content == "client_max_body_size")
		{
			t_token sizeToken = _expect(TOKEN_KEYWORD);
			newServer.clientMaxBodySize = std::strtoul(sizeToken.content.c_str(), NULL, 10);
			if (sizeToken.content.back() == 'M')
				newServer.clientMaxBodySize *= 1048576;
			_expect(TOKEN_SEMICOLON);
		}
		else if (directiveToken.content == "error_page")
		{
			std::vector<std::string> tempArgs;
			while (_currentToken().type == TOKEN_KEYWORD)
			{
				tempArgs.push_back(_currentToken().content);
				_advanceToken();
			}
			_expect(TOKEN_SEMICOLON);
			if (tempArgs.size() < 2)
        		throw std::runtime_error("Syntax error: error_page needs at least one code and a URI");
			std::string uri = tempArgs.back();
			tempArgs.pop_back();
			for (size_t i = 0; i < tempArgs.size(); ++i)
			{
				int errorCode = std::atoi(tempArgs[i].c_str());
				newServer.errorPages[errorCode] = uri;
			}
		}
		else if (directiveToken.content == "location")
		{
			t_token pathToken = _expect(TOKEN_KEYWORD);
			LocationConfig newLocation = _parseLocationBlock(pathToken.content);
			newServer.locations.push_back(newLocation);
		}
		else
		{
			std::stringstream ss;
			ss << "Syntax error at line " << directiveToken.line
				<< ": Unknown directive '" << directiveToken.content << "'";
			throw std::runtime_error(ss.str());
		}
	}
	_expect(TOKEN_RBRACE);
	return (newServer);
}

LocationConfig Parser::_parseLocationBlock(const std::string &path)
{
	LocationConfig newLocation;
	newLocation.path = path;
	_expect(TOKEN_LBRACE);
	while (_currentToken().type != TOKEN_RBRACE)
	{
		t_token directiveToken = _expect(TOKEN_KEYWORD);
		if (directiveToken.content == "root")
		{
			t_token rootToken = _expect(TOKEN_KEYWORD);
			newLocation.root = rootToken.content;
			_expect(TOKEN_SEMICOLON);
		}
		else if (directiveToken.content == "index")
		{
			t_token indexToken = _expect(TOKEN_KEYWORD);
			newLocation.index = indexToken.content;
			_expect(TOKEN_SEMICOLON);
		}
		else if (directiveToken.content == "autoindex")
		{
			t_token autoindexToken = _expect(TOKEN_KEYWORD);
			std::string autoindex = autoindexToken.content;
			if (autoindex != "on" && autoindex != "off")
				throw std::runtime_error("Syntax error: autoindex must be on/off");
			newLocation.autoindex = autoindex == "on" ?  true : false;
			_expect(TOKEN_SEMICOLON);
		}
		else if (directiveToken.content == "allow_methods")
		{
			while (_currentToken().type == TOKEN_KEYWORD)
			{
				newLocation.allowedMethods.push_back(_currentToken().content);
				_advanceToken();
			}
			_expect(TOKEN_SEMICOLON);
			if (newLocation.allowedMethods.empty())
				throw std::runtime_error("Syntax error: allow_methods directive is empty");
		}
		else if (directiveToken.content == "return")
		{
			t_token returnToken = _expect(TOKEN_KEYWORD);
			newLocation.returnCode = std::atoi(returnToken.content.c_str());
			returnToken = _expect(TOKEN_KEYWORD);
			newLocation.returnUrl = returnToken.content;
			_expect(TOKEN_SEMICOLON);
		}
		else if (directiveToken.content == "cgi")
		{
			t_token extToken = _expect(TOKEN_KEYWORD);
			t_token execToken = _expect(TOKEN_KEYWORD);
			newLocation.cgi[extToken.content] = execToken.content;
			_expect(TOKEN_SEMICOLON);
		}
		else if (directiveToken.content == "upload_store")
		{
			t_token uploadstoreToken = _expect(TOKEN_KEYWORD);
			newLocation.uploadStore = uploadstoreToken.content;
			_expect(TOKEN_SEMICOLON);
		}
		else
		{
			std::stringstream ss;
			ss << "Syntax error at line " << directiveToken.line
				<< ": Unknown locationdirective '" << directiveToken.content << "'";
		}
	}
	_expect(TOKEN_RBRACE);
	return (newLocation);
}

/*	----- Helpers ----- */

t_token Parser::_expect(e_token_type expected_type)
{
	if (this->_cursor >= _tokens.size())
		throw std::runtime_error("Syntax error: Unexpected end of file. Missing '}' or ';'");
	t_token current = _currentToken();
	if (current.type == expected_type)
	{
		_advanceToken();
		return (current);
	}
	std::stringstream ss;
    ss << "Syntax error at line " << _currentToken().line << ": Unexpected token '" 
		<< _currentToken().content<< "'";
    throw std::runtime_error(ss.str());
}

const t_token &Parser::_currentToken(void) const
{
	if (this->_cursor > this->_tokens.size())
		throw std::runtime_error("Parser error: Unexpected end of tokens");
	return (this->_tokens[this->_cursor]);
}

void Parser::_advanceToken(void)
{
	if (this->_cursor < this->_tokens.size())
		this->_cursor++;
}
