#pragma once
#include "../Network.hpp"

class ASimpleServer : public ListeningSocket
{
private:
    ListeningSocket *_socket;
    virtual void accepter() = 0;
    virtual void handler() = 0;
    virtual void responder() = 0;
public:
	ASimpleServer(int domain,int type,int protocol,int port,u_long ip,int backlog);
	virtual ~ASimpleServer(void);				// destructor

    virtual void launch() = 0;
    ListeningSocket *getSocket() const;

	ASimpleServer &operator=(ASimpleServer const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, ASimpleServer const &source);
