#pragma once
#include "../Network.hpp"

class Client
{
private:
    int _ClientFd;
    std::string _ReadBuffer;
    std::string _WriteBuffer;
    int _Status;
public:
    Client(/* args */);
    ~Client();
};

Client::Client(/* args */)
{
}

Client::~Client()
{
}
