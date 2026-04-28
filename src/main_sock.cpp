#include "../inc/Webserv.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
        return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
    (void)av;

    srand(std::time(NULL));
    std::signal(SIGINT, sigint_handler);
    try
    {
        Config config;

        //Criar um config e dar lhe a porta
        
        //ServerConfig config;

        // config.port = 8080;
        // Server t(config);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return (1);
    }
    return (0);
}
