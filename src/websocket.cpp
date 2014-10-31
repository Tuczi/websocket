#include "websocket.hpp"

void childend(int signo)
{
   pid_t pid;
   pid = wait(NULL);
   printf("\t[end of child process number %d]\n", pid);
}

void startServer(int& argc, char**& argv)
{
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
   static int buf_size = 10000;
   char buf[buf_size]={'\0'};
   tuczi::Websocket websocket(clientSocket);

   int status = websocket.read( (uint8_t*) buf, buf_size);
   //TODO printf or sth
   char* dataToSend = "Hello";
   websocket.write( (uint8_t*) dataToSend, 5);

   close(clientSocket);
   exit(0);
  }
}

namespace tuczi {

void Websocket::heandshakeResponce(std::string& keyResponce) {
   std::stringstream ss;
   ss<< "HTTP/1.1 101 Switching Protocols"<< LINE_END<<
    "Upgrade: websocket"<<LINE_END<<
    "Connection: Upgrade"<<LINE_END<<
    "Sec-WebSocket-Accept: "<<keyResponce<< LINE_END<<
    //"Sec-WebSocket-Protocol: chat" <<LINE_END<<
    LINE_END;

   std::string tmp=ss.str();
   printf("%s\n", tmp.c_str());
   ::write(descriptor, tmp.c_str(), tmp.size()-1);
}

std::string Websocket::encodeBase64(unsigned char input[SHA_DIGEST_LENGTH])
{
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, SHA_DIGEST_LENGTH);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  std::string result(bptr->length, '\0');
  memcpy((void*)result.c_str(), bptr->data, bptr->length-1);

  BIO_free_all(b64);

  return result;
}

std::string Websocket::parseHeandshake(std::string& input) {
    static std::string serverKeyToAppend = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string result;

    size_t pos = input.find("Sec-WebSocket-Key: ");
    pos+=19;
    size_t endPos = input.find("\n", pos);
    endPos--;

    result = input.substr(pos, endPos-pos);
    std::cout<<"key: \""<<result<<"\""<<std::endl;

    result = result + serverKeyToAppend;
    std::cout<<"appended: \""<<result<<"\""<<std::endl;

    unsigned char sha[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)result.c_str(), result.size() , sha);

   return encodeBase64(sha);
}

int Websocket::read(uint8_t* buf, size_t bufSize)  {
	int result = ::read(descriptor, buf, bufSize);
	return result - parseFrame(buf, bufSize);
}

int Websocket::write(uint8_t* buf, size_t bufSize)  {
	size_t headerSize;
	uint8_t* frameHeaderData = frameHeader(bufSize, headerSize);
	
	::write(descriptor, frameHeaderData, headerSize);
	return ::write(descriptor, buf, bufSize);
}

int Websocket::parseFrame(uint8_t* buf, size_t bufSize) {
  bool fin = buf[0] & (1<<7);
  bool rsv1 = buf[0] & (1<<6),
       rsv2 = buf[0] & (1<<5),
       rsv3 = buf[0] & (1<<4);
  uint8_t opcode = (buf[0] & 0x0f);
  bool mask = buf[1] & (1<<7);
  uint64_t size;
  uint8_t tmp = (uint8_t)buf[1] & ~(1<<7);
  uint8_t shift = 0;

  switch(tmp) {//TODO conwert network byte order to system
    case 126:
      size = ((uint64_t)buf[2]<<8) & uint64_t(buf[1]);
      shift=4;
      break;
    case 127:
      //TODO size
      shift=10;//the most significant bit MUST be 0)
      break;
    default:
      size = tmp;
      shift=2;
  }
  printf("fin: %d,rsv: %d %d %d, opcode: %d, mask?: %d, size(tmp): %d, size: %d\n",fin, rsv1, rsv2, rsv3, opcode, mask, tmp, size);

  uint8_t maskingKey[4];
  if(mask) {
    memcpy(maskingKey, buf+shift, 4);
    shift+=4;
  }
  uint64_t extension_data_length =0, application_data_length = size;//TODO

  uint8_t* data = (uint8_t*)buf+shift;

  if(mask)
    for(size_t i=0; i<(size_t)size;i++) {//TODO 64bit data
      buf[i]=data[i] ^ maskingKey[i%4];
      printf("%c",data[i]);
    }
    printf("\n");

  return shift;
}

uint8_t* Websocket::frameHeader(size_t bufSize, size_t& headerSize) {
  //TODO more then single frame
  //TODO size to network byte order
  uint8_t* header;
  if(bufSize <= 125) {
    headerSize=2;
    header = new uint8_t[headerSize];
    header[1]= bufSize;

  } else if( bufSize <= 0xffff ) {
    headerSize=4;
    header = new uint8_t[headerSize];
    header[1]=126;

    header[2]= (bufSize & 0xff00)>>8;
    header[3]= (bufSize & 0x00ff)>>8;
  } else {
    headerSize=10;
    header = new uint8_t[headerSize];
    header[1]=127;

    //TODO
  }
  //set fin bit, rsv1,2,3, opcode(text frame)
  //TODO more opcodes
  header[0]= 0x81;

  for(int i=0;i<headerSize;i++)
    printf("0x%x, ",header[i]);
  printf("\n");
  return header;
}

}
