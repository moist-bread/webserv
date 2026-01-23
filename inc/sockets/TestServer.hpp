#pragma once
#include "../Network.hpp"

class TestServer : public ASimpleServer
{
private:
    char _buffer[30000];
    int _newSocket;

    void accepter();
    void handler();
    void responder();
public:
	TestServer(void); 				// default constructor
	TestServer(TestServer const &source);	// copy constructor
	~TestServer(void);				// destructor

    void launch();

	TestServer &operator=(TestServer const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, TestServer const &source);
