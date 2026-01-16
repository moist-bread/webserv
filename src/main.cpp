#include "../inc/Class.hpp"

int main(int ac, char **av)
{
	if (ac != 2)
		return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
	(void)av;
	return (0);
}
