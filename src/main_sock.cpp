#include "../inc/Webserv.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
        return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
    (void)av;

    std::signal(SIGINT, sigint_handler);
    Server t;
    
    return (0);
}
