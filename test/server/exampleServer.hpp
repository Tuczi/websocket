#ifndef TUCZI_TEST_EXAmPLE_SERVER_HPP
#define TUCZI_TEST_EXAmPLE_SERVER_HPP

#include "../../src/websocket.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>

#include <fstream>
#include <opencv2/opencv.hpp>

#define SERVER_PORT 9000
#define QUEUE_SIZE 5

void childend(int signo);
void startServer(int& argc, char**& argv);
void serveClient(int clientSocket);

void textTest(int);
void imgTest(int);
void videoTest(int);
#endif
