#include "../../inc/sockets/FileDescriptor.hpp"

#include <unistd.h> // close

FileDescriptor::FileDescriptor(int fd) : _fd(fd) {}

FileDescriptor::FileDescriptor(FileDescriptor const &src) {	*this = src; }

FileDescriptor &FileDescriptor::operator=(FileDescriptor const &src)
{
	if (this != &src)
		setFd(_fd);
	return (*this);
}

FileDescriptor::~FileDescriptor(void)
{
	if (this->_fd >= 2)
		close(_fd);
}

int FileDescriptor::getFd() const { return _fd; }

void FileDescriptor::setFd(int fd) { this->_fd = fd; }

FileDescriptor::operator int() const { return (_fd); }
