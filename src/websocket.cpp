#include "websocket.hpp"

namespace tuczi {

bool Websocket::init() {
  HeandshakeCtx heandshakeCtx;
  std::string buffer(HEANDSHAKE_BUF_SIZE,'\0');

  ::read(descriptor, (void*) buffer.c_str(), HEANDSHAKE_BUF_SIZE);
std::cout<<buffer;
  parseHeandshake(buffer, heandshakeCtx);
std::cout<<"parse end\n";

  //valid request
  if(std::find(heandshakeCtx.versions.begin(), heandshakeCtx.versions.end(),13)==heandshakeCtx.versions.end()) {
    return false;
  }

  heandshakeResponce(heandshakeCtx);
  return true;
}

void Websocket::heandshakeResponce(HeandshakeCtx& heandshakeCtx) {
   std::stringstream ss;
   ss<< "HTTP/1.1 101 Switching Protocols"<< LINE_END<<
    "Upgrade: websocket"<<LINE_END<<
    "Connection: Upgrade"<<LINE_END<<
    "Sec-WebSocket-Accept: "<<heandshakeCtx.responceKey<< LINE_END<<
    "Sec-WebSocket-Version: 13"<< LINE_END<<
    //"Sec-WebSocket-Protocol: chat" <<LINE_END<<
    LINE_END;

   std::string tmp=ss.str();
   printf("%s\n", tmp.c_str());

   ::write(descriptor, tmp.c_str(), tmp.size()-1);
}

std::string Websocket::encodeBase64(unsigned char (&input)[SHA_DIGEST_LENGTH])
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

void Websocket::parseHeandshake(std::string& buffer, HeandshakeCtx& heandshakeCtx) {
  static std::string serverKeyToAppend = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  std::string tmp;
  size_t pos, endPos;
  //key
  pos = buffer.find("Sec-WebSocket-Key: ");
  pos+=19;
  endPos = buffer.find("\n", pos);
  endPos--;

  tmp = buffer.substr(pos, endPos-pos);
  std::cout<<"key: \""<<tmp<<"\""<<std::endl;

  tmp += serverKeyToAppend;
  std::cout<<"appended: \""<<tmp<<"\""<<std::endl;

  unsigned char sha[SHA_DIGEST_LENGTH];
  SHA1((const unsigned char *)tmp.c_str(), tmp.size() , sha);
  heandshakeCtx.responceKey = encodeBase64(sha);

  //versions
  pos = buffer.find("Sec-WebSocket-Version: ");
  pos+=23;
  endPos = buffer.find("\n", pos);
  endPos--;

  do {
    heandshakeCtx.versions.push_back(atoi(buffer.c_str()+pos));
    pos = buffer.find(",", pos)+1;
  } while( pos<endPos && pos != std::string::npos );

  //TODO protocols
}

bool Websocket::read_(uint8_t* buf, size_t bufSize, size_t& bytesRead) {
  bytesRead = ::read(descriptor, buf, bufSize);
  size_t shift = 0;
  if(readCtx.frameSize == 0) { //new frame
    readCtx = ReadCtx();
    shift = parseFrame(buf, bufSize);
  }
  uint8_t* data = (uint8_t*)buf+shift;
  if(readCtx.mask && shift)
    for(size_t i=0; i<(size_t)bytesRead-shift; i++)
      buf[i]=data[i] ^ readCtx.maskingKey[(i + readCtx.maskingKeyIter)%4];
  else if(shift)
    for(size_t i=0; i<(size_t)bytesRead-shift;i++)
      buf[i]=data[i];
  else if(readCtx.mask)
    for(size_t i=0; i<(size_t)bytesRead;i++)
      buf[i]=buf[i] ^ readCtx.maskingKey[(i + readCtx.maskingKeyIter)%4];

  if(readCtx.mask)
    readCtx.maskingKeyIter = (readCtx.maskingKeyIter + bytesRead-shift)%4;

  bytesRead-=shift;
  readCtx.frameSize -= bytesRead;

  return readCtx.frameSize != 0;
}

bool Websocket::write_(uint8_t* buf, size_t bufSize, size_t& bytesWritten) {
  if(writeCtx.frameSize <= bufSize)
    bytesWritten = ::write(descriptor, buf, writeCtx.frameSize);
  else
    bytesWritten = ::write(descriptor, buf, bufSize);

  writeCtx.frameSize -= bytesWritten;
  return writeCtx.frameSize!=0;
}

bool Websocket::writeHeader(size_t dataSize, Opcode dataType) {
  uint8_t frameHeaderData[MAX_HEADER_SIZE];
  size_t headerSize, bytesWritten = 0;

  writeCtx.frameSize = dataSize;
  frameHeader(dataSize, dataType, frameHeaderData, headerSize);

  while(bytesWritten != headerSize)
    bytesWritten += ::write(descriptor, frameHeaderData+bytesWritten, headerSize-bytesWritten);

  return true;
}

size_t Websocket::parseFrame(uint8_t* buf, size_t bufSize) {
  readCtx.fin = buf[0] & (1<<7);
  bool rsv1 = buf[0] & (1<<6),
       rsv2 = buf[0] & (1<<5),
       rsv3 = buf[0] & (1<<4);
  readCtx.opcode = (buf[0] & 0x0f);
  readCtx.mask = buf[1] & (1<<7);
  uint8_t tmp = (uint8_t)buf[1] & ~(1<<7);
  uint8_t shift = 0;

  switch(tmp) {
    case 126:
      readCtx.frameSize = ntohs( buf[2] | buf[3]<<8 );
      shift = 4;
      break;
    case 127:
      readCtx.frameSize = 0;
      for(int i=0;i<8;i++)
        readCtx.frameSize|=(uint64_t(buf[2+i])<< uint64_t(8*i));
      readCtx.frameSize = be64toh(readCtx.frameSize);
      shift = 10; //TODO the most significant bit MUST be 0)
      break;
    default:
      readCtx.frameSize = tmp;
      shift = 2;
  }
  printf("fin: %d,rsv: %d %d %d, opcode: %d, mask?: %d, size(tmp): %d, size: %ld %d\n", readCtx.fin, rsv1, rsv2, rsv3, readCtx.opcode, readCtx.mask, tmp, readCtx.frameSize, buf[2]<<16 | buf[3]<<8 | buf[4] );

  if(readCtx.mask) {
    memcpy(readCtx.maskingKey, buf+shift, 4);
    shift+=4;
  }
  //uint64_t extension_data_length =0, application_data_length = size;//TODO
  return shift;
}

void Websocket::frameHeader(size_t bufSize, Opcode opcode, uint8_t (&header)[MAX_HEADER_SIZE], size_t& headerSize) {
  //TODO more then single frame
  if(bufSize <= 125) {
    headerSize = 2;
    header[1]= bufSize;

  } else if( bufSize <= 0xffff ) {
    headerSize = 4;
    header[1] = 126;

    uint16_t tmp = htobe16(bufSize);
    header[2] = uint8_t(tmp);
    header[3] = uint8_t(tmp >> 8);
  } else {
    headerSize = 10;
    header[1] = 127;

    uint64_t tmp = htobe64(bufSize);
    for(int i=0;i<8;i++)
      header[2+i] = uint8_t(tmp >> (8*i));
  }
  //set fin bit, rsv1,2,3, opcode
  header[0]= 0x80 | opcode;

  for(int i=0;i<headerSize;i++)
    printf("0x%x, ",header[i]);
  printf("\n");
}

bool Websocket::readPart(void* buffer, size_t bufferSize, size_t& dataRead) {
  size_t tmp=0, read=0;
  bool status=true;

  for(;status && read!= bufferSize; read+=tmp)
   status = read_((uint8_t*)(buffer)+read, bufferSize-read, tmp);

  dataRead=read;
  return status;
}

bool Websocket::writePart(void* buffer, size_t bufferSize) {
  size_t tmp=0, written=0;
  bool status=true;

  for(;status && written!=bufferSize; written+=tmp)
    status = write_((uint8_t*)(buffer)+written, bufferSize-written, tmp);

  return status;
}

bool Websocket::read(void*& buffer, size_t& bufferSize) {
  uint8_t header[100];
  size_t byteRead=0;
  read_( header, 100, byteRead );

  bufferSize = readCtx.frameSize>0? readCtx.frameSize : byteRead;
  bool isText = (readCtx.opcode == Opcode::TEXT);
  bufferSize += isText;
  buffer = new uint8_t[bufferSize];
  ((uint8_t*)(buffer))[bufferSize-1] = '\0';

  memcpy(buffer, header, byteRead);

  if(readCtx.frameSize == 0) //frame end - no more data
    return true;

  size_t tmp;
  return readPart((uint8_t*)(buffer)+byteRead, bufferSize-isText, tmp);
}

Websocket::Frame Websocket::read() {
  uint8_t header[100];
  size_t byteRead=0,tmp;
  read_( header, 100, byteRead );

  size_t bufferSize = readCtx.frameSize>0? readCtx.frameSize : byteRead;
  bool isText = (readCtx.opcode == Opcode::TEXT);
  if(isText) {
    std::string buffer(bufferSize, '\0');
    memcpy((void*)buffer.c_str(), header, byteRead);

    if(readCtx.frameSize) //frame not end - no more data
      readPart((uint8_t*)(buffer.c_str())+byteRead, bufferSize, tmp);

    return Frame(buffer);
  }

  uint8_t* buffer = new uint8_t[bufferSize];
  memcpy(buffer, header, byteRead);

  if(readCtx.frameSize) //frame not end - no more data
    readPart(buffer + byteRead, bufferSize, tmp);

  return Frame(buffer, bufferSize);
}

}

