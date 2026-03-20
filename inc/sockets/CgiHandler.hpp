#pragma once
#include "../Network.hpp"
#include "../requests/HTTP.hpp"
#include <sstream>

class CgiHandler
{
private:
    int _pipeIn[2];  // Tubo 1: O C++ escreve (1), o Python lê (0 - STDIN)
    int _pipeOut[2]; // Tubo 2: O Python escreve (1 - STDOUT), o C++ lê (0)
    int _pid;

    std::string _query;
    std::string _body;
    std::string _scriptPath;
    t_method _method;
    std::string _compiler;

    int writeBodyToCgiInput();

public:
    CgiHandler(Request &src);
    int InitPipe();
    int getPipeOutReadFd() const;
    int executeCgi();

    ~CgiHandler();
};