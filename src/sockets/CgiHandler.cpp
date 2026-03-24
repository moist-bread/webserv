#include "../../inc/sockets/CgiHandler.hpp"

extern std::string method_names[];

CgiHandler::CgiHandler(void) {}

CgiHandler::CgiHandler(CgiHandler const &source)
{
	*this = source;
}

CgiHandler::~CgiHandler(void) {}

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
void CgiHandler::process(Request &src)
{
	clear();
	update_info(src);
	executeCgi();
}

void CgiHandler::clear()
{
	_pipeIn[0] = 0;
	_pipeIn[1] = 0;
	_pipeOut[0] = 0;
	_pipeOut[1] = 0;
	_pid = 0;
	_env.clear();
	_body.clear();
	_scriptPath.clear();
	_compiler.clear();
}

void CgiHandler::update_info(Request &src)
{
	_body = src.body;
	_scriptPath = ("www" + src.path_uri);
	this->_compiler = "/usr/bin/python3"; // FROM CONFIG

	std::string empty;
	// -- server info
	// from config
	_env.push_back("SERVER_SOFTWARE=" + empty);
	_env.push_back("SERVER_NAME=" + empty);
	_env.push_back("SERVER_PROTOCOL=" + empty);
	_env.push_back("SERVER_PORT=" + empty);
	_env.push_back("SERVER_ADMIN=" + empty);
	_env.push_back("GATEWAY_INTERFACE=" + empty);
	_env.push_back("DOCUMENT_ROOT=" + empty);

	// -- all the headers from the http request
	for (map_strings::iterator it = src.headers.begin(); it != src.headers.end(); it++)
	{
		// add "HTTP_" before all keys, the "-" become "_" and all uppercase
		std::string key = (*it).first;
		std::transform(key.begin(), key.end(), key.begin(), ::screaming_snake_case);
		_env.push_back("HTTP_" + key + "=" + (*it).second);
	}

	_env.push_back("REQUEST_METHOD=" + method_names[src.method]);
	_env.push_back("PATH=" + empty);
	_env.push_back("QUERY_STRING=" + src.query);
	_env.push_back("REMOTE_ADDR=" + empty);
	_env.push_back("REMOTE_HOST=" + empty);
	_env.push_back("REMOTE_PORT=" + empty);
	_env.push_back("REMOTE_USER=" + empty);
	_env.push_back("REQUEST_URI=" + _scriptPath);
	_env.push_back("SCRIPT_FILENAME=" + empty);
	_env.push_back("SCRIPT_NAME=" + _scriptPath);
	_env.push_back("CONTENT_LENGTH=" + ft_to_string(_body.size()));
	_env.push_back("CONTENT_TYPE=" + empty);
}

int CgiHandler::executeCgi()
{
	if (InitPipes() == -1)
		return -1; // WHAT TO DO IN CASE OF -1????

	this->_pid = fork();
	if (_pid == -1)
	{
		std::cout << "Something went wrong while doing fork" << std::endl;
		return -1;
	}
	else if (_pid >= 1)
	{
		std::cout << "------Parent process ------" << std::endl;

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

		// -- CREATING THE ENV FOR THE CGI
		char **envp = NULL;
		try
		{
			envp = create_execve_env();
		}
		catch (const std::bad_alloc &e)
		{
			std::cerr << e.what() << std::endl;
		}

		// testing the env
		std::cerr << "\nCgi env:\n";
		for (size_t i = 0; i < _env.size(); i++)
			std::cerr << envp[i] << std::endl;

		execve(argv[0], argv, envp);
		perror("execve failed");

		for (size_t i = 0; i < _env.size(); i++)
			delete[] envp[i];
		delete[] envp;
		_exit(EXIT_FAILURE);
	}
	return 1;
}

int CgiHandler::InitPipes()
{
	if (pipe(this->_pipeIn) == -1)
	{
		std::cout << "Something went wrong while doing pipe" << std::endl;
		return -1;
	}

	if (pipe(this->_pipeOut) == -1)
	{
		std::cout << "Something went wrong while doing pipe" << std::endl;
		return -1;
	}
	return 0;
}

int CgiHandler::writeBodyToCgiInput() const
{
	if (!_body.empty())
	{
		ssize_t n = write(this->_pipeIn[1], _body.c_str(), _body.size());
		if (n <= 0)
		{
			std::cout << "write() failed" << std::endl;
			close(this->_pipeIn[1]);
			return -1;
		}
	}

	// if we verify CLOSE SUCCESS HERE we should to the same everywhere
	if (close(this->_pipeIn[1]) == -1)
	{
		std::cout << "close(_pipeIn[1]) failed" << std::endl;
		return -1;
	}

	return 0;
}

char **CgiHandler::create_execve_env(void) const
{
	char **exec_env = new char *[(_env.size() + 1)];
	for (size_t i = 0; i < _env.size(); i++)
	{
		exec_env[i] = new char[_env[i].length() + 1];
		std::strcpy(exec_env[i], _env[i].c_str());
	}
	exec_env[_env.size()] = NULL;
	return (exec_env);
}

int CgiHandler::getPipeOutReadFd() const
{
	return this->_pipeOut[0];
}
