#include "../../inc/sockets/CgiHandler.hpp"
#include "../../inc/requests/Request.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include "../../inc/serverConfig/LocationConfig.hpp"

#include <unistd.h>		// close, fork, pipe
#include <ctime>		// time
#include <fstream>		// fstream, perror
#include <algorithm>	// transform, EXIT_FAILURE

CgiHandler::CgiHandler(void) : time_started(VALUE_NOT_SET) {}

CgiHandler::CgiHandler(CgiHandler const &source)
{
	*this = source;
}

CgiHandler::~CgiHandler(void) { clear(); }

CgiHandler &CgiHandler::operator=(CgiHandler const &source)
{
	if (this != &source)
	{
		(void)source;
	}
	return (*this);
}

int screaming_snake_case(int i)
{
	if (i == '-')
		return '_';
	return toupper(i);
}

CgiHandler::CgiHandler(Request &src)
{
	process(src);
}
void CgiHandler::process(Request &req)
{
	std::cout << "-- THIS IS A CGI!!!!!!!" << std::endl;
	clear();
	update_info(req);
	executeCgi();
}

void CgiHandler::clear()
{
	_pipeIn[0] = 0;
	_pipeIn[1] = 0;
	_pipeOut[0] = 0;
	_pipeOut[1] = 0;
	_pid = 0;
	_body.clear();
	_compiler.clear();
	_scriptPath.clear();
	_env.clear();
	time_started = VALUE_NOT_SET;
}

void CgiHandler::update_info(Request &req)
{
	_body = req.body;
	std::string filename = extract_script_filename(req.path_uri);
	_scriptPath = req.conf->getRoot() + filename;

	std::fstream fs(_scriptPath.c_str(), std::fstream::in);
	if (!fs.is_open())
		throw(CgiHandler::CgiExecutionFail());

	this->_compiler = req.loc->getCgiExecutable(req.file_extension);

	std::string empty;
	_env.push_back("SERVER_SOFTWARE=" + to_str("server/1.0.0"));
	_env.push_back("SERVER_NAME=" + req.conf->getServerNames()[0]); // -------------------------
	_env.push_back("SERVER_PROTOCOL=" + HTTP::stringProtocol(req.protocol));
	_env.push_back("SERVER_PORT=" + to_str(req.conf->getListenPort()));
	_env.push_back("GATEWAY_INTERFACE=" + to_str("CGI/1.1"));

	_env.push_back("REQUEST_METHOD=" + HTTP::stringMethod(req.method));
	_env.push_back("PATH_INFO=" + extract_path_info(req.path_uri));
	_env.push_back("QUERY_STRING=" + req.query);
	_env.push_back("SCRIPT_NAME=" + _scriptPath);
	_env.push_back("SCRIPT_FILENAME=" + filename);
	_env.push_back("REMOTE_ADDR=" + req.headers["x-forwarded-for"]);
	_env.push_back("REQUEST_URI=" + req.path_uri + req.query);
	_env.push_back("CONTENT_LENGTH=" + to_str(_body.size()));
	_env.push_back("CONTENT_TYPE=" + req.headers["content-type"]);

	for (map_strings::iterator it = req.headers.begin(); it != req.headers.end(); it++)
	{
		// add "HTTP_" before all keys, the "-" become "_" and all uppercase
		std::string key = (*it).first;
		std::transform(key.begin(), key.end(), key.begin(), ::screaming_snake_case);
		_env.push_back("HTTP_" + key + "=" + (*it).second);
	}
}

std::string CgiHandler::extract_script_filename(std::string full_path)
{
	// script filename is everything until the end of the script extention
	size_t pos = full_path.find(".py"); // OR ANY OTHER THING WE DECIDE FOR THE CGI
	if (pos == std::string::npos)
		return (full_path);
	else if (pos + 3 < full_path.size())
		return (full_path.substr(0, pos + 3));
	else
		return (full_path);
}

std::string CgiHandler::extract_path_info(std::string full_path)
{
	// "path info is whatever comes after the program name in the url"
	size_t pos = full_path.rfind(".py"); // OR ANY OTHER THING WE DECIDE FOR THE CGI
	if (pos == std::string::npos)
		return (full_path);
	else if (pos + 3 < full_path.size())
		return (full_path.substr(pos + 3));
	else
		return (to_str("/"));
}

int CgiHandler::executeCgi()
{
	if (InitPipes() == -1)
		throw(std::runtime_error("pipe failure"));

	this->_pid = fork();
	if (_pid == -1)
		throw(std::runtime_error("fork failure"));
	else if (_pid >= 1)
	{
		std::cout << "------ Parent process ------" << std::endl;

		close(this->_pipeIn[0]);
		close(this->_pipeOut[1]);
		if (writeBodyToCgiInput() == -1)
			return -1;
	}
	else if (0 == this->_pid)
	{
		std::cout << "------ I'm the child process------" << std::endl;

		// Vou usar o dup2 para mudar onde o meu texto vai ser displayed
		dup2(this->_pipeIn[0], STDIN_FILENO);
		dup2(this->_pipeOut[1], STDOUT_FILENO);

		close(this->_pipeIn[1]);
		close(this->_pipeOut[0]);

		char *argv[3];
		argv[0] = const_cast<char *>(_compiler.c_str());
		argv[1] = const_cast<char *>(this->_scriptPath.c_str());
		argv[2] = NULL;

		std::vector<char *> exec_env;
		for (size_t i = 0; i < _env.size(); i++)
			exec_env.push_back(const_cast<char *>(_env[i].c_str()));
		exec_env.push_back(NULL);

		execve(argv[0], argv, &exec_env[0]);
		perror("execve failed");
		_exit(EXIT_FAILURE);
		time_started = VALUE_NOT_SET;
	}
	time_started = std::time(NULL);
	return 1;
}

int CgiHandler::InitPipes()
{
	if (pipe(this->_pipeIn) == -1)
		return -1;

	if (pipe(this->_pipeOut) == -1)
		return -1;
	return 0;
}

int CgiHandler::writeBodyToCgiInput() const // does this really need to retyurn when we are throwing?
{
	if (!_body.empty())
	{
		ssize_t n = write(this->_pipeIn[1], _body.c_str(), _body.size());
		if (n <= 0)
		{
			close(this->_pipeIn[1]);
			throw(std::runtime_error("write failure"));
			return -1;
		}
	}

	// if we verify CLOSE SUCCESS HERE we should to the same everywhere
	if (close(this->_pipeIn[1]) == -1) // !!!!!!!!!!!!!!!!
	{
		std::cout << "close(_pipeIn[1]) failed" << std::endl;
		throw(std::runtime_error("close failure"));
		return -1;
	}

	return 0;
}

int CgiHandler::getPipeOutReadFd() const
{
	return this->_pipeOut[0];
}

time_t CgiHandler::getCgiActivityStart(void) const
{
	return (time_started);
}
