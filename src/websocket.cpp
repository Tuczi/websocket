#include "websocket.hpp"

#define LINE_END "\n\r"
#include <string>
#include <sstream>
void test(){
  std::string tmp ="Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n";
  std::cout<< parseClientHeandShake(tmp)<<std::endl;

  //equal to: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
}

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

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
   test();//remove
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
   read(clientSocket, buf, buf_size);
   printf(buf);

   std::string tmp(buf);
   std::string keyResponce=parseClientHeandShake(tmp);

   std::stringstream ss;
   ss<< "HTTP/1.1 101 Switching Protocols"<< LINE_END<<
    "Upgrade: websocket"<<+LINE_END<<
    "Connection: Upgrade"<<LINE_END<<
    "Sec-WebSocket-Accept: "<<keyResponce<< LINE_END<<
    //"Sec-WebSocket-Protocol: chat" <<LINE_END<<
    LINE_END;

   tmp = ss.str();
   printf("%s\n", tmp.c_str());
   write(clientSocket, tmp.c_str(), tmp.size());

   memset(buf,0,buf_size);
   int s =read(clientSocket,buf,buf_size);
   uint8_t* msg = parseMsg((uint8_t*)buf, s);
   //printf("%s\n", msg);

   //write(clientSocket,"OK",2);

   close(clientSocket);
   exit(0);
  }
}

std::string parseClientHeandShake(std::string& input) {
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

   return base64_encode(sha, SHA_DIGEST_LENGTH);
}

uint8_t* parseMsg(const uint8_t* buf, size_t bufSize) {
  bool fin = buf[0] & (1<<7);
  bool rsv1 = buf[0] & (1<<6),
       rsv2 = buf[0] & (1<<5),
       rsv3 = buf[0] & (1<<4);
  uint8_t opcode = (buf[0] & 0xf0);//not shifted
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

  uint8_t* maskingKey = nullptr;
  if(mask) {
    maskingKey = (uint8_t*)buf+shift;
    shift+=4;
  }
  uint64_t extension_data_length =0, application_data_length = size;//TODO

  uint8_t* data = (uint8_t*)buf+shift;

  if(mask)
    for(size_t i=0; i<(size_t)size;i++) {//TODO 64bit data
      data[i]=data[i] ^ maskingKey[i%4];
      printf("%c",data[i]);
    }
    printf("\n");

  return data;
}
