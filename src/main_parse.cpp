#include "../inc/Class.hpp"
#include "../inc/Network.hpp"
#include "../inc/serverConfig/Config.hpp"

int main(int ac, char **av)
{
	if (ac != 2)
		return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
	try {
		Config webServerConfig;
		webServerConfig.load(av[1]);
		std::cout << webServerConfig << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
