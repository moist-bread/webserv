#include "../inc/Class.hpp"
#include "../inc/Network.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
        return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
    (void)av;

    std::cout << "Starting server..." << std::endl;
    std::cout << "Creating listening socket on port 8080..." << std::endl;
    ListeningSocket server(AF_INET, SOCK_STREAM, 0, 8080, INADDR_ANY, 10);
    std::cout << "Success! Server is listening..." << std::endl;
    
   
    
    return (0);
}
