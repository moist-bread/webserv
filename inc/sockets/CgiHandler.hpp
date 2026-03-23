#pragma once
#include "../Network.hpp"
#include "../requests/HTTP.hpp"
#include <sstream>

class CgiHandler
{
private:
	int _pipeIn[2];  // Tubo 1: O C++ escreve (1), o Python lê (0 - STDIN)
	int _pipeOut[2]; // Tubo 2: O Python escreve (1 - STDOUT), o C++ lê (0)
	int _pid;

	std::vector<std::string> _env;
	std::string _body;
	std::string _scriptPath;
	std::string _compiler;

	int writeBodyToCgiInput();

public:
	CgiHandler(Request &src);
	int InitPipe();
	int getPipeOutReadFd() const;
	int executeCgi();
	char **create_execve_env(void);

	~CgiHandler();
};

template <typename T> std::string ft_to_string(const T& value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
};
