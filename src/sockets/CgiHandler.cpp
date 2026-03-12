#include "CgiHandler.hpp"


CgiHandler::CgiHandler(const std::string& ScriptPath, const std::string& body, const std::string& method) 
    : _scriptPath(ScriptPath), _body(body), _method(method)
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
    size_t total = 0;
    const char* data = this->_body.c_str();
    size_t len = this->_body.size();

    while (total < len)
    {
        ssize_t n = write(this->_pipeIn[1], data + total, len - total);
        if (n <= 0)
        {
            std::cout << "write() failed" << std::endl;
            close(this->_pipeIn[1]);
            return -1;
        }
        total += static_cast<size_t>(n);
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


        // Converter o tamanho do body para string 
        std::stringstream ss;
        ss << "CONTENT_LENGTH=" << this->_body.size();
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
    }
    return 1;
}

CgiHandler::~CgiHandler()
{
}