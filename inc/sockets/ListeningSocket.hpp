#pragma once

#include "BindingSocket.hpp"

class ListeningSocket : public BindingSocket
{
protected:
	int _backlog;

private:
	int listening;

public:
	ListeningSocket(int domain, int type, int protocol, int port, u_long ip, int backlog);
	ListeningSocket(ListeningSocket const &source);
	ListeningSocket &operator=(ListeningSocket const &source);
	~ListeningSocket(void);

	void start_listening();
};
