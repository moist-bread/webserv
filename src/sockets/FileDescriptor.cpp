#include "../../inc/sockets/FileDescriptor.hpp"

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
    if(this->_fd >= 0)
    {
        close(_fd);
        std::cout << "Socket " << this->_fd << " has been close automatically" << std::endl;
    }
	std::cout << GRN "the FileDescriptor ";
	std::cout << URED "has been deleted" DEF << std::endl;
}

std::ostream &operator<<(std::ostream &out, FileDescriptor const &source)
{
	(void)source;
	out << BLU "FileDescriptor";
	out << DEF << std::endl;
	return (out);
}

int FileDescriptor::getFd() const{
    return _fd;
}

void FileDescriptor::setFd(int fd)
{
    this->_fd = fd;
}

FileDescriptor::operator int() const {
    return _fd;
}