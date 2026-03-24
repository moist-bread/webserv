#pragma once
#include "../requests/HTTP.hpp"
#include <unistd.h>
#include <sstream>
#include <vector>
#include <cstring>

class CgiHandler
{
private:
	int _pipeIn[2];	 // Tubo 1: O C++ escreve (1), o Python lê (0 - STDIN)
	int _pipeOut[2]; // Tubo 2: O Python escreve (1 - STDOUT), o C++ lê (0)
	int _pid;

	std::vector<std::string> _env;
	std::string _body;
	std::string _scriptPath;
	std::string _compiler;

	void clear();
	void update_info(Request &src);
	int executeCgi();
	int InitPipes();
	int writeBodyToCgiInput() const;
	char **create_execve_env(void) const;

public:
	CgiHandler(void);
	CgiHandler(CgiHandler const &source);
	~CgiHandler(void);
	CgiHandler &operator=(CgiHandler const &source);

	CgiHandler(Request &src);

	void process(Request &src);
	int getPipeOutReadFd() const;
};
