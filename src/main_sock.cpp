#include "../inc/Webserv.hpp"

#include <csignal>
void sigint_handler(int sig);

int main(int ac, char **av)
{
    if (ac != 2)
        return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
    (void)av;

    srand(time(NULL));
    std::signal(SIGINT, sigint_handler);
    try
    {
        Server t;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return (1);
    }
    return (0);
}
