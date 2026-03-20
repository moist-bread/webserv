#include "../../inc/sockets/CgiHandler.hpp"

extern std::string method_names[];

CgiHandler::CgiHandler(Request &src) 
    : _query(src.query), _body(src.body), _method(src.method)
{
    _scriptPath = ("www" + src.path_uri);
    this->_compiler = "/usr/bin/python3"; // FROM CONFIG
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
        char *envp[4];
        //std::vector<char *> build_env;


        // Converter o tamanho do body para string 
        std::stringstream ss;
        ss << "CONTENT_LENGTH=" << _body.size();
        std::string contentLengthEnv = ss.str();
        std::string requestmethod = "REQUEST_METHOD=" + method_names[_method];
        std::string querystring = "QUERY_STRING=" + _query;
        envp[0] = const_cast<char *>(requestmethod.c_str());
        envp[1] = const_cast<char *>(querystring.c_str());
        envp[2] = const_cast<char *>(contentLengthEnv.c_str()); 
        envp[3] = NULL;
        execve(argv[0],argv,envp);
        perror("execve failed");
        _exit(EXIT_FAILURE);
    }
    return 1;
}

CgiHandler::~CgiHandler()
{
}