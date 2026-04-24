#include "../../inc/sockets/FileDescriptor.hpp"

#include <iostream>
#include <unistd.h>

#include "../../inc/ansi_color_codes.h"

FileDescriptor::FileDescriptor(void) : _fd(-1)
{
	std::cout << GRN "the FileDescriptor ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

FileDescriptor::FileDescriptor(int fd) : _fd(fd)
{
	std::cout << GRN "the FileDescriptor ";
	std::cout << UCYN "has been created" DEF << std::endl;
}

FileDescriptor::~FileDescriptor(void)
{
	if (this->_fd >= 2)
	{
		close(_fd);
		std::cout << "Socket " << this->_fd << " has been closed automatically" << std::endl;
	}
}
FileDescriptor::FileDescriptor(FileDescriptor const &src)
{
	*this = src;
}
FileDescriptor &FileDescriptor::operator=(FileDescriptor const &src)
{
	if (this != &src)
		setFd(_fd);
	return (*this);
}

int FileDescriptor::getFd() const
{
	return _fd;
}

void FileDescriptor::setFd(int fd)
{
	this->_fd = fd;
}

FileDescriptor::operator int() const
{
	return (_fd);
}
