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
    videoTest(clientSocket);
  }
}

void textTest(int clientSocket) {
  tuczi::Websocket websocket(clientSocket);

  if(!websocket.init()){
    std::cerr<<"websocket heandshake error\n";
    exit(0);
  }
  bool status;
  uint8_t* buf;
  size_t bufSize, byteCounter;

  status = websocket.read( (void*&) buf, bufSize);
  printf("status: %d, msg: %s", status, buf);
  delete[] buf;

  std::string dataToSend = "Hello";
  status = websocket.write( (void*)dataToSend.c_str(), bufSize );

  status = websocket.read( (void*&) buf, bufSize);
  printf("status: %d, msg: %s", status, buf);
  delete[] buf;

  close(clientSocket);
  exit(0);
}

void imgTest(int clientSocket) {
  bool status;
  uint8_t* readBuf;
  static size_t writeBufSize = 1024;
  uint8_t writeBuf[writeBufSize];
  size_t bufSize;

  tuczi::Websocket websocket(clientSocket);

  if(!websocket.init()) {
    std::cerr<<"websocket heandshake error\n";
    exit(0);
  }

  while(true) {
    size_t byteCounter;
    status = websocket.read( (void*&)readBuf, bufSize);

    std::string name("test/server/resources/");
    name = name + (const char*) readBuf;
    delete[] readBuf;

    std::fstream file(name, std::ios_base::in| std::ios_base::binary);
    std::cout<<name<<" File exists?"<< file.good()<<std::endl;
    file.seekg (0, file.end);
    int length = file.tellg();
    file.seekg (0, file.beg);

    status = websocket.writeHeader( length, tuczi::Websocket::Opcode::BINARY );
    printf("writeHeader - status %d, size %d\n", status, length);

    size_t sendedBytes=0;
    if(status) {
      while(file) {
        file.read((char*)writeBuf, writeBufSize);
        size_t c = file ? writeBufSize : file.gcount();
        std::cout<<"c: "<<c<<std::endl;
        status = websocket.writePart( (void*)writeBuf, c);
        printf("WRITE - byteCounter: %d, status: %d\n", byteCounter, status);
      }
    }
    file.close();
    printf("File closed\n");
  }
  close(clientSocket);
  exit(0);
}

void videoTest(int clientSocket) {
  bool status;
  size_t bufSize;
  uint8_t* buf;

  tuczi::Websocket websocket(clientSocket);

  if(!websocket.init()) {
    std::cerr<<"websocket heandshake error\n";
    exit(0);
  }

  while(true) {
    size_t byteCounter=0;

    status = websocket.read( (void*&)buf, bufSize);
    printf("READ - buf szie: %d,status: %d, readed: %s,\n", bufSize, status, buf);

    std::string name("test/server/resources/");
    name = name + (const char*)buf;
    delete[] buf;

    cv::VideoCapture videoCap;
    videoCap.open(name);
    std::cout<<name<<" File exists?"<< videoCap.isOpened()<<std::endl;

    cv::Mat frame;
    while(videoCap.isOpened()) {
      status = videoCap.read(frame); // read a new frame from video
      if (!status) { //if not success, break loop
        std::cout << "Cannot read the frame from video file" << std::endl;
        break;
      }

      printf("R: %d, C%d\n", frame.rows, frame.cols);
      size_t frameSize = frame.rows * frame.cols*3;

      status = websocket.write( (void*)frame.data, frameSize, tuczi::Websocket::Opcode::BINARY );
      printf("write - status %d, size: %d\n", status, frameSize);
    }

    std::cout<<"LOOP END"<<std::endl;
    break;
  }

  close(clientSocket);
  exit(0);
}

