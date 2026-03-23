#include "../../inc/sockets/CgiHandler.hpp"

extern std::string method_names[];

CgiHandler::CgiHandler(Request &src) 
	: _body(src.body)
{
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
	// add "HTTP_" before all of them and all "-" become "_"
	_env.push_back("HTTP_COOKIE=" + empty);
	_env.push_back("HTTP_HOST=" + empty);
	_env.push_back("HTTP_REFERER=" + src.headers["referer"]);
	_env.push_back("HTTP_USER_AGENT=" + empty);
	_env.push_back("HTTPS=" + empty);
	
	
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

int CgiHandler::InitPipe()
{
	if(pipe(this->_pipeIn) == -1)
	{
		std::cout << "Something went wrong while doing pipe" << std::endl;
		return -1;
	}

	if(pipe(this->_pipeOut) == -1)
	{
		std::cout << "Something went wrong while doing pipe" << std::endl;
		return -1;
	}
	return 0;
}

int CgiHandler::writeBodyToCgiInput()
{
	if (!_body.empty())
	{
		ssize_t n = write(this->_pipeIn[1], _body.c_str() , _body.size());
		if (n <= 0)
		{
			std::cout << "write() failed" << std::endl;
			close(this->_pipeIn[1]);
			return -1;
		}
	}

	if (close(this->_pipeIn[1]) == -1)
	{
		std::cout << "close(_pipeIn[1]) failed" << std::endl;
		return -1;
	}

	return 0; 
}

int CgiHandler::getPipeOutReadFd() const
{
	return this->_pipeOut[0]; 
}

int CgiHandler::executeCgi()
{
	if(InitPipe() == -1)
		return -1;

	this->_pid = fork();

	if(_pid == -1)
	{
		std::cout << "Something went wrong while doing fork" << std::endl;
		return -1;
	}
	else if(_pid >= 1)
	{
		std::cout << "------Parent process ------" << std::endl;

		close(this->_pipeIn[0]);
		close(this->_pipeOut[1]);

		// writing body to cgi stdinput
		if (writeBodyToCgiInput() == -1)
			return -1;
	}
	//Filho
	else if(0 == this->_pid)
	{
		std::cout << "------ I'm the child process------" << std::endl;

		//Vou usar o dup2 para mudar onde o meu texto vai ser displayed
		dup2(this->_pipeIn[0],STDIN_FILENO);
		dup2(this->_pipeOut[1],STDOUT_FILENO);

		close(this->_pipeIn[1]);
		close(this->_pipeOut[0]);

		
		char* argv[3];
		argv[0] = const_cast<char *>(_compiler.c_str());
		argv[1] = const_cast<char *>(this->_scriptPath.c_str());
		argv[2] = NULL;
		
		// -- CREATING THE ENV FOR THE CGI

		char **envp = NULL;
		try
		{
			envp = create_execve_env();
		}
		catch(const std::bad_alloc &e)
		{
			std::cerr << e.what() << std::endl;
		}
		
		// testing the env
		for (size_t i = 0; i < _env.size(); i++)
			std::cerr << envp[i] << std::endl;
		sleep (50);

		execve(argv[0], argv, envp);
		perror("execve failed");

		for (size_t i = 0; i < _env.size(); i++)
			delete [] envp[i];
		delete [] envp;
		_exit(EXIT_FAILURE);
	}
	return 1;
}

char **CgiHandler::create_execve_env(void)
{
	char **exec_env = new char *[(_env.size() + 1)];
	for (size_t i = 0; i < _env.size(); i++)
	{
		exec_env[i] = new char[_env[i].length() + 1];
  		std::strcpy (exec_env[i], _env[i].c_str());
	}
	exec_env[_env.size()] = NULL;
	return (exec_env);
}

CgiHandler::~CgiHandler()
{
}
