#pragma once

class FileDescriptor
{
private:
	int _fd;
	
public:
	FileDescriptor(int fd);
	FileDescriptor(FileDescriptor const &src);
	FileDescriptor &operator=(FileDescriptor const &src);
	~FileDescriptor(void);

	int getFd() const;
	void setFd(int fd);
	operator int() const;
};
