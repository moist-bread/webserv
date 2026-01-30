#include "../../inc/sockets/SocketController.hpp"
#include "../../inc/ansi_color_codes.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
/*
    [X]Criar uma socket
    [X] fazer bind para obter o socket address(IPV4 + PORT)
*/


// SocketController::SocketController(void)
// {
// 	std::cout << GRN "the SocketController ";
// 	std::cout << UCYN "has been created" DEF << std::endl;
// }


    //Vou criar a socket
    /*
    
    domain
    Escolhe qual maneira de comunicacao se vai usar.Escolhi AF_INET porque e que usa os IPv4 que vai ser preciso para a socket futuramente acho eu LMAO

    type 
    Pede que tipo de esquema e que queres que a tua socket comunique,
     eu escolhi a SOCK_STREAM porque e a mais completa e segura

    protocol
    Faz com que a tua socket possa ter comportamentos diferentes do normal (ativa flags que estao no manual)
    por enquanto vamos manter desativado para testar 

    */


SocketController::SocketController(int domain,int type,int protocol,int port,u_long ip)
	: _sock(socket(domain,type,protocol))
{
	std::cout << GRN "the SocketController ";
	std::cout << UCYN "has been created" DEF << std::endl;

    //Init na estrutura de network para configurar a conexao feita atraves do bind
    this->_address.sin_family = domain;
    /*
    temos de usar htons para organizar os bites de port para se encaxarem bem na network
    */
   this->_address.sin_port = htons(port);
   //IPV4 da maquina
   this->_address.sin_addr.s_addr = htonl(ip);
    test_connection(_sock);
}

void SocketController::test_connection(int test_connection)
{
    if(0 > test_connection)
    {
        perror("Trying to connect...");
        exit(EXIT_FAILURE);
    }
}

SocketController::SocketController(SocketController const &source)
{
	*this = source;
	std::cout << GRN "the SocketController ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}


SocketController &SocketController::operator=(SocketController const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, SocketController const &source)
{
	
	out << BLU "SocketController " << "_sockedfd value: " << source.getSocketfd();
	out << DEF << std::endl;
	return (out);
}

int SocketController::getSocketfd(void) const{
    return this->_sock;
}


 struct sockaddr_in SocketController::getStructAdress(void)const
 {
    return _address;
 }


SocketController::~SocketController(void)
{
	std::cout << GRN "the SocketController ";
	std::cout << URED "has been deleted" DEF << std::endl;
}