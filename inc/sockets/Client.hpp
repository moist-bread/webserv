#pragma once
#include "../Network.hpp"

class Client
{
private:
    int _ClientFd;
    std::string _requestBuffer; //Informaçao que estou a receber
    std::string _respondBuffer; //Informaçao que estou a receber

    //Post
    bool _readAllHeaders; //Verifica se ja passei dos headers
    size_t  _headerBytes; //tamanho dos headers
    size_t  _contentLength; //tamanho do body

    //
    time_t _lastActivity;
public:
    Client(void); 				// default constructor
	Client(int fd); 				// default constructor
	Client(Client const &source);	// copy constructor
	~Client(void);				// destructor

    Response response;
    
    //Appending 
    void feed(const char* data, int size);

    void extractContentLength();

    //Getters
    std::string GetRequestBuffer() const;
    std::string GetWriteBuffer() const;
    time_t GetLastActivity() const;

    void updateLastActivity();

    ///Setter
    void SetRespondBuffer(const std::string str);
    //Clean up
    void ClearRequestBuffer();
    void ClearRespondBuffer();

    //Erase
    void EraseParte(int start,int idx);

    //bool
    bool requestFullyReceived();

	Client &operator=(Client const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, Client const &source);

