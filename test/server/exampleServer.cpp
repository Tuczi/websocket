#include "exampleServer.hpp"

void childend(int signo)
{
   fprintf(stderr, "exiting with signal %d\n", signo);
   pid_t pid;
   pid = wait(NULL);
   printf("\t[end of child process number %d]\n", pid);
}

void startServer(int& argc, char**& argv)
{
   if(argc<1) exit(1);

   int nSocket, nClientSocket;
   int nBind, nListen;
   int nFoo = 1;
   socklen_t nTmp;
   struct sockaddr_in stAddr, stClientAddr;

   signal(SIGCHLD, childend);

   /* address structure */
   memset(&stAddr, 0, sizeof(struct sockaddr));
   stAddr.sin_family = AF_INET;
   stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   stAddr.sin_port = htons(SERVER_PORT);

   /* create a socket */
   nSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (nSocket < 0)
   {
       fprintf(stderr, "%sCan't create a socket.\n", argv[0]);
       exit(1);
   }
   setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&nFoo, sizeof(nFoo));

   /* bind a name to a socket */
   nBind = bind(nSocket, (struct sockaddr*)&stAddr, sizeof(struct sockaddr));
   if (nBind < 0)
   {
       fprintf(stderr, "%s: Can't bind a name to a socket.\n", argv[0]);
       exit(1);
   }

   /* specify queue size */
   nListen = listen(nSocket, QUEUE_SIZE);
   if (nListen < 0)
   {
       fprintf(stderr, "%s: Can't set queue size.\n", argv[0]);
   }
   while(1)
   {
       /* block for connection request */
       nTmp = sizeof(struct sockaddr);
       nClientSocket = accept(nSocket, (struct sockaddr*)&stClientAddr, &nTmp);
       if (nClientSocket < 0)
       {
           fprintf(stderr, "%s: Can't create a connection's socket.\n", argv[0]);
           exit(1);
       }

       /* connection */
       serveClient(nClientSocket);
   }

   close(nSocket);
}

void serveClient(int clientSocket) {
  if(!fork()) {
    bool status;
    static int buf_size = 100000;
    char buf[buf_size] = {'\0'};
    tuczi::Websocket websocket(clientSocket);

    size_t byteCounter;
    do{
	  status = websocket.read( (uint8_t*) buf, buf_size, byteCounter );
	  buf[byteCounter]='\0';
      printf("READ - byteCounter %d, status: %d, readed: %s,\n", byteCounter, status, buf);
     } while(status);

    std::string dataToSend = "Hello";
    status = websocket.writeHeader( dataToSend.size() );
    printf("writeHeader - status %d\n", status);

    if(status) {
	  size_t shift=0;
	  do {
        status = websocket.write( (uint8_t*) dataToSend.c_str()+shift, dataToSend.size()-shift, byteCounter) ;
        shift+=byteCounter;
        printf("WRITE - byteCounter: %d, status: %d\n", byteCounter, status);
      } while(status);
    }

    close(clientSocket);
    exit(0);
  }
}
