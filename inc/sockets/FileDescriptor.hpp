#pragma once

class FileDescriptor
{
private:
	int _fd;
	FileDescriptor(FileDescriptor const &src);
	FileDescriptor &operator=(FileDescriptor const &src);

public:
	FileDescriptor(void);
	FileDescriptor(int fd);
	~FileDescriptor(void);

	int getFd() const;
	void setFd(int fd);
	operator int() const;
};
