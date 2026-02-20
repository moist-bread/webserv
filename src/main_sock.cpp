#include "../inc/Class.hpp"
#include "../inc/Network.hpp"
#include "../inc/sockets/Server.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
        return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
    (void)av;

    
    Server t;
    
    return (0);
}
