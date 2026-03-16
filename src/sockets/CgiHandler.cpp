#include "../../inc/sockets/CgiHandler.hpp"


CgiHandler::CgiHandler(const std::string& ScriptPath, const  std::map<std::string, std::string>& query, const std::string& method) 
    : _query(query), _scriptPath(ScriptPath), _method(method)
{
    this->_compiler = "/usr/bin/python3";
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
    // very inefficient, will later try not to parse the querys so much as to keep this simple
    std::map<std::string, std::string>::iterator end = _query.end();
    for (std::map<std::string, std::string>::iterator begin = _query.begin(); begin != end; begin++)
    {
        ssize_t n = write(this->_pipeIn[1], (*begin).first.c_str() , (*begin).first.size());
        if (n <= 0)
        {
            std::cout << "write() failed" << std::endl;
            close(this->_pipeIn[1]);
            return -1;
        }
        n = write(this->_pipeIn[1], "=" , 1);
        if (n <= 0)
        {
            std::cout << "write() failed" << std::endl;
            close(this->_pipeIn[1]);
            return -1;
        }
        n = write(this->_pipeIn[1], (*begin).second.c_str() , (*begin).second.size());
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

        ssize_t n = 0;
        std::map<std::string, std::string>::iterator end = _query.end();
        for (std::map<std::string, std::string>::iterator begin = _query.begin(); begin != end; begin++)
        {
            n += (*begin).first.size() + 1 + (*begin).second.size();
        }
        // Converter o tamanho do body para string 
        std::stringstream ss;
        ss << "CONTENT_LENGTH=" << n;
        std::string contentLengthEnv = ss.str();

        char* argv[3];
        argv[0] = const_cast<char *>(_compiler.c_str());
        argv[1] = const_cast<char *>(this->_scriptPath.c_str());
        argv[2] = NULL;

        char *envp[3];
        envp[0] = const_cast<char *>(this->_method.c_str());
        envp[1] = const_cast<char *>(contentLengthEnv.c_str()); 
        envp[2] = NULL;
        execve(argv[0],argv,envp);
        perror("execve failed");
        _exit(EXIT_FAILURE);
    }
    return 1;
}

CgiHandler::~CgiHandler()
{
}