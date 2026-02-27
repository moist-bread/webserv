#include "../inc/Webserv.hpp"

int main(int ac, char **av)
{
	if (ac != 2)
		return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
	(void)av;

	try
	{
		Request test(av[1]);
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
	return (0);
}
