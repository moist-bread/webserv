#pragma once
#include "../Network.hpp"
#include "CgiHandler.hpp"

class Client
{
private:
    int _ClientFd;
    time_t _lastActivity;

public:
    Client(void);                 // default constructor
    Client(int fd);               // default constructor
    Client(Client const &source); // copy constructor
    ~Client(void);                // destructor

    Request request;
    Response response;
    CgiHandler cgi;

    // Getters
    time_t GetLastActivity() const;
    int GetClientFd() const;

    void updateLastActivity();

    Client &operator=(Client const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, Client const &source);
