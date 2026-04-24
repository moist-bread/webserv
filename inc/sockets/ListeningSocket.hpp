#pragma once

#include "BindingSocket.hpp"

class ListeningSocket : public BindingSocket
{
protected:
	int _backlog;

private:
	int listening;
	// -- MISSING DEFAULT CONSTRUCTOR
	// ListeningSocket(void) {}; // default constructor

public:
	ListeningSocket(int domain, int type, int protocol, int port, u_long ip, int backlog); // default constructor
	ListeningSocket(ListeningSocket const &source);										   // copy constructor
	~ListeningSocket(void);																   // destructor

	void start_listening();

	ListeningSocket &operator=(ListeningSocket const &source); // copy assignment operator overload
};
