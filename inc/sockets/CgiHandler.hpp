#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <ctime>

class Request;

class CgiHandler
{
private:
	int _pipeIn[2];	 // Pipe 1: Server writes to index 1, Child (Script) reads from index 0 (STDIN).
	int _pipeOut[2]; // Pipe 2: Child (Script) writes to index 1 (STDOUT), Server reads from index 0.
	int _pid; //Process ID of the spawned CGI child process

	std::string _body; //The HTTP request body to be sent to the CGI script via STDIN.
	std::string _compiler; //Path to the script interpreter (e.g., /usr/bin/python3).
	std::string _scriptPath; //Absolute path to the CGI script to be executed.
	std::vector<std::string> _env; //Formatted environment variables for the script

	time_t time_started; //Timestamp of when the CGI execution began (used for timeouts).

	void clear();
	void update_info(Request &req);
	void executeCgi();
	int InitPipes();
	int writeBodyToCgiInput() const;
	static std::string extract_script_filename(const std::string full_path, const std::string ext);

public:
	CgiHandler(void);
	CgiHandler(CgiHandler const &src);
	~CgiHandler(void);
	CgiHandler &operator=(CgiHandler const &src);

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
