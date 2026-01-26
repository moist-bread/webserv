#pragma once

// ==┊ needed libs by class
#include <iostream>
#include <unistd.h>
#include "../../inc/ansi_color_codes.h"


// =====>┊( CLASS )┊

class FileDescriptor
{
private:
    int _fd;
public:
	FileDescriptor(int fd); 				// default constructor (com valor default)
	FileDescriptor(void); 				// default constructor (com valor default)
	FileDescriptor(FileDescriptor const &source);	// copy constructor
	~FileDescriptor(void);				// destructor

    int getFd() const;
    void setFd(int fd);
	operator int() const;  // operador de conversão para int
	FileDescriptor &operator=(FileDescriptor const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, FileDescriptor const &source);
