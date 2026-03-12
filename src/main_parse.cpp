#include "../inc/Webserv.hpp"

int main(int ac, char **av)
{
	if (ac != 2)
		return (std::cout << "usage: ./webserv [configuration file]" << std::endl, 2);
	(void)av;

	Request recieved;
	Response reply;
	t_status_code parse_status = OK;
	try
	{
		recieved.process(av[1]);
	}
	catch (Request::ParseError &e)
	{
		std::cerr << e.what() << std::endl;
		parse_status = e.request_status;
	}
	try
	{
		reply.process(recieved, parse_status);
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}
	return (0);
}
