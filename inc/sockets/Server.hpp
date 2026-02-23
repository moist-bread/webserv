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

class Server : public ASimpleServer
{
private:


    std::map<int,Client>_clients;
    std::vector<struct pollfd> _pollfds; // vou dar store no fd das sockets que vao ser criadas quando alguem se conectar

    std::vector<int> _serverPorts;//Fds de todas as portas abertas
    std::vector<ListeningSocket*> _extraListeners;
    std::vector<int> _listeningFds;

    void  SetNonblocking(int fd);
    void accepter(int listenFd);
    void handler(std::string buffer);
    int responder(int clientFd,const std::string& data);
public:

	Server(void); 				// default constructor
	Server(Server const &source);	// copy constructor
	~Server(void);				// destructor


    void removeClient(int fd, size_t& index);

    void launch();
    
    //Setup Servers Ports
    void SetupPorts();

    //Opens a file 
    std::string OpenFile(const std::string& path);

    //
    bool isServerSocket(int fd);

    void PopulatePollInfo(int fd);
	Server &operator=(Server const &source); // copy assignment operator overload
};

enum ConnectionStatus {
    IO_ERROR,
    IO_CLOSED,
    IO_DATA_READY,
    IO_DATA_OUT
};

std::ostream &operator<<(std::ostream &out, Server const &source);
ConnectionStatus getStatus(int ret);
