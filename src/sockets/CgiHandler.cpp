#include "../../inc/sockets/CgiHandler.hpp"
#include "../../inc/requests/Request.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include "../../inc/serverConfig/LocationConfig.hpp"
#include "../../inc/string_utils.tpp"

#include <unistd.h>		// close, fork, pipe
#include <ctime>		// time
#include <fstream>		// fstream
#include <algorithm>	// transform, EXIT_FAILURE

CgiHandler::CgiHandler(void) : time_started(VALUE_NOT_SET) {}

CgiHandler::CgiHandler(CgiHandler const &src) {	*this = src; }

CgiHandler::~CgiHandler(void) { clear(); }

CgiHandler &CgiHandler::operator=(CgiHandler const &src)
{
	if (this != &src)
	{
		(void)src;
	}
	return (*this);
}

int screaming_snake_case(int i)
{
	if (i == '-')
		return '_';
	return toupper(i);
}

CgiHandler::CgiHandler(Request &req)
{
	process(req);
}
void CgiHandler::process(Request &req)
{
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
	
	setCgiActivityStart(VALUE_NOT_SET);
}

void CgiHandler::update_info(Request &req)
{
	_body = req.body;
	// std::string filename = extract_script_filename(req.path_uri, req.file_extension);
	// _scriptPath = req.conf->getRoot() + filename;
	if (!req.loc->isCgiPass())
		_scriptPath = req.loc->getRoot() + req.path_uri.substr(req.loc->getPath().length());
	else
	{
		_scriptPath = req.loc->getRoot();
		size_t pos = req.path_uri.rfind("." + req.file_extension);
		if (pos == std::string::npos)
			_scriptPath += req.path_uri;
		else
		{
			pos = req.path_uri.rfind("/", pos);
			if (pos == std::string::npos)
				_scriptPath += req.path_uri;
			else
				_scriptPath += req.path_uri.substr(pos + 1);
		}
	}

	std::cout << BLU "SCRIPT PATH: " DEF << _scriptPath << std::endl;
	// std::cout << BLU "SCRIPT FILENAME: " DEF << filename << std::endl;
	// std::cout << BLU "PATH INFO: " DEF << extract_path_info(req.path_uri, req.file_extension) << std::endl;
	std::fstream fs(_scriptPath.c_str(), std::fstream::in);
	if (!fs.is_open())
		throw(CgiHandler::CgiExecutionFail("open failure"));

	_compiler = req.loc->getCgiExecutable(req.file_extension);
	// std::cout << BLU "COMPILER: " DEF << _compiler << std::endl;

	// std::string empty;
	_env.push_back("SERVER_SOFTWARE=" + to_str("server/1.0.0"));
	_env.push_back("SERVER_NAME=" + req.conf->getServerNames()[0]); // -------------------------
	_env.push_back("SERVER_PROTOCOL=" + HTTP::stringProtocol(req.protocol));
	_env.push_back("SERVER_PORT=" + to_str(req.conf->getListenPort()));
	_env.push_back("GATEWAY_INTERFACE=" + to_str("CGI/1.1"));

	_env.push_back("REQUEST_METHOD=" + HTTP::stringMethod(req.method));
	_env.push_back("PATH_INFO=" + extract_path_info(req.path_uri, req.file_extension));
	_env.push_back("QUERY_STRING=" + req.query);
	_env.push_back("SCRIPT_NAME=" + _scriptPath);
	_env.push_back("SCRIPT_FILENAME=" + extract_script_filename(req.path_uri, req.file_extension));
	
	if (req.headers.find("x-forwarded-for") != req.headers.end())
		_env.push_back("REMOTE_ADDR=" + req.headers["x-forwarded-for"]);
	_env.push_back("REQUEST_URI=" + req.path_uri + req.query);
	_env.push_back("CONTENT_LENGTH=" + to_str(_body.size()));
	if (req.headers.find("content-type") != req.headers.end())
		_env.push_back("CONTENT_TYPE=" + req.headers["content-type"]);

	for (map_strings::const_iterator it = req.headers.begin(); it != req.headers.end(); it++)
	{
		// add "HTTP_" before all keys, the "-" become "_" and all uppercase
		std::string key = (*it).first;
		std::transform(key.begin(), key.end(), key.begin(), ::screaming_snake_case);
		_env.push_back("HTTP_" + key + "=" + (*it).second);
	}
}

std::string CgiHandler::extract_script_filename(const std::string full_path, const std::string ext)
{
	// script filename is everything until the end of the script extention
	size_t pos = full_path.rfind("." + ext);
	if (pos == std::string::npos)
		return (full_path);
	return (full_path.substr(0, pos + ext.length() + 1));
}

std::string CgiHandler::extract_path_info(const std::string full_path, const std::string ext)
{
	// path info is whatever comes after the program name in the url
	size_t pos = full_path.rfind("." + ext);
	if (pos == std::string::npos)
		return (full_path);
	if (pos + ext.length() + 1 == full_path.size())
		return (to_str("/"));
	return (full_path.substr(pos + ext.length() + 1));
}

int CgiHandler::executeCgi()
{
	if (InitPipes() == -1)
		throw(CgiHandler::CgiExecutionFail("pipe failure"));

	this->_pid = fork();
	std::cerr << "my pid: " << this->_pid << std::endl;
	if (this->_pid == -1)
		throw(CgiHandler::CgiExecutionFail("fork failure"));
	else if (this->_pid >= 1)
	{
		close(this->_pipeIn[0]);
		close(this->_pipeOut[1]);
		if (writeBodyToCgiInput() == -1)
			return -1;
	}
	else if (this->_pid == 0)
	{
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
		{
			exec_env.push_back(const_cast<char *>(_env[i].c_str()));
			std::cerr << _env[i] << std::endl;
		}
		exec_env.push_back(NULL);

		std::cerr << "GO EXECUTE" << std::endl;
		execve(argv[0], argv, &exec_env[0]);
		std::cerr << "FAILED TO EXECUTE" << std::endl;
		setCgiActivityStart(VALUE_NOT_SET);
		// _exit(EXIT_FAILURE);
		throw(CgiHandler::CgiExecutionFail("execve failure"));
	}
	setCgiActivityStart(std::time(NULL));
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

int CgiHandler::writeBodyToCgiInput() const // does this really need to return when we are throwing?
{
	if (!_body.empty())
	{
		std::cerr << RED "going to WRITE...\n\n" DEF;
		ssize_t n = write(this->_pipeIn[1], _body.c_str(), _body.size());
		if (n <= 0)
		{
			std::cerr << RED "FAILED WRITE...\n\n" DEF;
			close(this->_pipeIn[1]);
			throw(CgiHandler::CgiExecutionFail("write failure"));
			return -1;
		}
		std::cerr << RED "SUCCESS WRITE...\n\n" DEF;
	}

	// if we verify CLOSE SUCCESS HERE we should to the same everywhere
	if (close(this->_pipeIn[1]) == -1) // !!!!!!!!!!!!!!!!
	{
		std::cerr << "close(_pipeIn[1]) failed" << std::endl;
		throw(CgiHandler::CgiExecutionFail("close failure"));
		return -1;
	}

	return 0;
}

int CgiHandler::getPipeOutReadFd() const { return this->_pipeOut[0]; }

const time_t &CgiHandler::getCgiActivityStart(void) const { return (time_started); }

void CgiHandler::setCgiActivityStart(const time_t &t) { time_started = t; }
