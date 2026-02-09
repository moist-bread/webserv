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
	Client(void); 				// default constructor
	Client(Client const &source);	// copy constructor
	~Client(void);				// destructor

	Client &operator=(Client const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, Client const &source);
