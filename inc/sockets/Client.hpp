#pragma once
#include "../Network.hpp"

class Client
{
private:
    int _ClientFd;
    std::string _requestBuffer; //Informaçao que estou a receber
    std::string _respondBuffer; //Informaçao que vou enviar 
    int _Status; //Status das ligaçoes
public:
    Client(void); 				// default constructor
	Client(int fd); 				// default constructor
	Client(Client const &source);	// copy constructor
	~Client(void);				// destructor

    //Appending 
    void feed(const char* data, int size);

    //Getters
    std::string GetRequestBuffer() const;
    std::string GetWriteBuffer() const;

    ///Setter
    void SetRespondBuffer(const std::string& str);
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

