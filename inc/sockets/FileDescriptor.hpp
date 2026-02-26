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
	FileDescriptor(FileDescriptor const &source);
	FileDescriptor &operator=(FileDescriptor const &source);
public:
	FileDescriptor(int fd);
	FileDescriptor(void);
	~FileDescriptor(void);

    int getFd() const;
    void setFd(int fd);
	operator int() const;
};

std::ostream &operator<<(std::ostream &out, FileDescriptor const &source);
