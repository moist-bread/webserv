#pragma once
#include "../Network.hpp"

/*

1)
    [X] Importar biblotecas
    [X] criar vetor fds em
    [X] criar funcao para nao bloquear o servidor


    Tenho de fazer  Nonblocking Sockets

    Sockets normais vao congelar quando um cliente demora a enviar dados e isso faz com que a socket para de processar ficando assim a dormir
    also o subject diz que temos de ter Nonblocking 

2)Loop principal
    [X] Chamar funcao de anti bloqueio dentro do launch
    [X] Pupular a estrutura do poll


*/


class TestServer : public ASimpleServer
{
private:
    std::string _buffer;
    int _newSocket;

    std::vector<struct pollfd> _pollfds; // vou dar store no fd das sockets que vao ser criadas quando alguem se conectar
    void  SetNonblocking(int fd);
    void accepter();
    void handler();
    void responder(int clientFd);
public:
	TestServer(void); 				// default constructor
	TestServer(TestServer const &source);	// copy constructor
	~TestServer(void);				// destructor

    void launch();
    
    void PopulatePollInfo(int fd);
	TestServer &operator=(TestServer const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, TestServer const &source);
void signalHandler();