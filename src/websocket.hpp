#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>

#include <openssl/sha.h>
#define LINE_END "\n\r"
#include <string>
#include <sstream>


#define SERVER_PORT 9000
#define QUEUE_SIZE 5

void childend(int signo);
void startServer(int& argc, char**& argv);
void serveClient(int clientSocket);

namespace tuczi {
class Websocket {
  private:
    const int descriptor;
    
    static const size_t BUF_SIZE = 1024;

    std::string parseHeandshake(std::string& input);
    void heandshakeResponce(std::string& keyResponce);
    
    int parseFrame(uint8_t * buffer, size_t bufferSize);
    uint8_t* frameHeader(size_t dataSize, size_t& headerSize);
	
  public:
    Websocket(int descriptor_): descriptor(descriptor_) { 
	  std::string buffer(BUF_SIZE, 0);
      ::read(descriptor, (void*) buffer.c_str(), BUF_SIZE);
      std::string key = parseHeandshake(buffer);
      heandshakeResponce(key);
	}

    int read(uint8_t* buffer, size_t bufferSize);
    int write(uint8_t* buffer, size_t bufferSize);
};
}

