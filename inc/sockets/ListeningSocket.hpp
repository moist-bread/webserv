#pragma once
#include "../Network.hpp"

class ListeningSocket : public BindingSocket
{
	protected:
		int _backlog;
	private:
		int listening;
public:
	ListeningSocket(int domain, int type,int protocol,int port,u_long ip,int backlog); 				// default constructor
	ListeningSocket(ListeningSocket const &source);	// copy constructor
	~ListeningSocket(void);				// destructor

	void start_listening();

	ListeningSocket &operator=(ListeningSocket const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, ListeningSocket const &source);