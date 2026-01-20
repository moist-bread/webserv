#include "../../inc/Class.hpp"

Class::Class(void)
{
	std::cout << GRN "the Class ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

Class::Class(Class const &source)
{
	*this = source;
	std::cout << GRN "the Class ";
	std::cout << UYEL "has been copy created" DEF << std::endl;
}

Class::~Class(void)
{
	std::cout << GRN "the Class ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

Class &Class::operator=(Class const &source)
{
	std::cout << YEL "copy assignment operator overload..." DEF << std::endl;
	if (this != &source)
		(void)source;
	return (*this);
}

std::ostream &operator<<(std::ostream &out, Class const &source)
{
	(void)source;
	out << BLU "Class";
	out << DEF << std::endl;
	return (out);
}
