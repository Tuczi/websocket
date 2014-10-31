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

#define SERVER_PORT 9000
#define QUEUE_SIZE 5

void childend(int signo);
void startServer(int& argc, char**& argv);
void serveClient(int clientSocket);

std::string parseClientHeandShake(std::string&);
uint8_t* parseFrame(const uint8_t *, size_t size);
uint8_t* frameHeader(size_t dataSize, size_t& headerSize);
