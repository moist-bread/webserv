#pragma once

#include <string>
#include <vector>
#include <exception>

class Request;

class CgiHandler
{
private:
	int _pipeIn[2];	 // Tubo 1: O C++ escreve (1), o Python lê (0 - STDIN)
	int _pipeOut[2]; // Tubo 2: O Python escreve (1 - STDOUT), o C++ lê (0)
	int _pid;

	std::string _body;
	std::string _compiler;
	std::string _scriptPath;
	std::vector<std::string> _env;

	void clear();
	void update_info(Request &src);
	int executeCgi();
	int InitPipes();
	int writeBodyToCgiInput() const;
	char **create_execve_env(void) const;
	static std::string extract_script_filename(std::string full_path);
	static std::string extract_path_info(std::string full_path);

public:
	CgiHandler(void);
	CgiHandler(CgiHandler const &source);
	~CgiHandler(void);
	CgiHandler &operator=(CgiHandler const &source);

	CgiHandler(Request &src);

	void process(Request &src);
	int getPipeOutReadFd() const;
	time_t getCgiActivityStart(void) const;

	time_t time_started; // use to detect CGI timeout
	
	class CgiExecutionFail : public std::exception
	{
	public:
		const char *what(void) const throw() { return ("execution failure"); }
	};
};
