#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <ctime>

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

	time_t time_started;

	void clear();
	void update_info(Request &req);
	int executeCgi();
	int InitPipes();
	int writeBodyToCgiInput() const;
	static std::string extract_script_filename(const std::string full_path, const std::string ext);
	// static std::string extract_path_info(const std::string full_path, const std::string ext);

public:
	CgiHandler(void);
	CgiHandler(CgiHandler const &src);
	~CgiHandler(void);
	CgiHandler &operator=(CgiHandler const &src);

	CgiHandler(Request &req);

	void process(Request &req);
	int getPipeOutReadFd() const;
	const time_t &getCgiActivityStart(void) const;
	void setCgiActivityStart(const time_t &t);
	
	class CgiExecutionFail : public std::runtime_error
	{
	public:
		CgiExecutionFail(const std::string &msg) : runtime_error("failed to execute due to: " + msg) {};
	};
};
