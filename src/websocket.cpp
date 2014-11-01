#include "websocket.hpp"

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

bool Websocket::read(uint8_t* buf, size_t bufSize, size_t& bytesRead) {
  bytesRead = ::read(descriptor, buf, bufSize);
  size_t shift = 0;
  if(readData.frameSize == 0) { //new frame
    shift = parseFrame(buf, bufSize);
  }
  uint8_t* data = (uint8_t*)buf+shift;

  if(readData.mask && shift)
    for(size_t i=0; i<(size_t)bytesRead-shift; i++) //TODO 64bit data
      buf[i]=data[i] ^ readData.maskingKey[i%4];
  else if(shift)
    for(size_t i=0; i<(size_t)bytesRead-shift;i++)
      buf[i]=data[i];

  bytesRead-=shift;
  readData.frameSize -= bytesRead;

  if(readData.frameSize ==0) { //end of frame
    readData.mask=false;
  }

  return readData.frameSize != 0;
}

bool Websocket::write(uint8_t* buf, size_t bufSize, size_t& bytesWritten) {
  if(writeFrameSize <= bufSize) 
    bytesWritten = ::write(descriptor, buf, writeFrameSize);
  else 
	bytesWritten = ::write(descriptor, buf, bufSize);
  
  writeFrameSize -= bytesWritten;
  return writeFrameSize!=0;
}

bool Websocket::writeHeader(size_t dataSize, uint8_t dataType) {
  //TODO not dynamic alloc
  //TODO dataType
  //TODO check write
  size_t headerSize;
  writeFrameSize = dataSize;
  uint8_t* frameHeaderData = frameHeader(dataSize, headerSize);

  size_t bytesWritten = ::write(descriptor, frameHeaderData, headerSize);
  delete[] frameHeaderData;

  return bytesWritten == headerSize;
}

size_t Websocket::parseFrame(uint8_t* buf, size_t bufSize) {
  readData.fin = buf[0] & (1<<7);
  bool rsv1 = buf[0] & (1<<6),
       rsv2 = buf[0] & (1<<5),
       rsv3 = buf[0] & (1<<4);
  uint8_t opcode = (buf[0] & 0x0f);
  readData.mask = buf[1] & (1<<7);
  uint8_t tmp = (uint8_t)buf[1] & ~(1<<7);
  uint8_t shift = 0;

  switch(tmp) {
    case 126:
      readData.frameSize = ntohs( buf[2]<<8 | buf[3] );
      shift = 4;
      break;
    case 127:
      readData.frameSize = 0;
      for(int i=0;i<8;i++)
        readData.frameSize|=(buf[2+i]<<(56-8*i));
      readData.frameSize = be64toh(readData.frameSize);
      shift = 10; //TODO the most significant bit MUST be 0)
      break;
    default:
      readData.frameSize = tmp;
      shift = 2;
  }
  printf("fin: %d,rsv: %d %d %d, opcode: %d, mask?: %d, size(tmp): %d, size: %ld\n", readData.fin, rsv1, rsv2, rsv3, opcode, readData.mask, tmp, readData.frameSize);

  if(readData.mask) {
    memcpy(readData.maskingKey, buf+shift, 4);
    shift+=4;
  }
  //uint64_t extension_data_length =0, application_data_length = size;//TODO
  return shift;
}

uint8_t* Websocket::frameHeader(size_t bufSize, size_t& headerSize) {
  //TODO more then single frame
  uint8_t* header;
  if(bufSize <= 125) {
    headerSize = 2;
    header = new uint8_t[headerSize];
    header[1]= bufSize;

  } else if( bufSize <= 0xffff ) {
    headerSize = 4;
    header = new uint8_t[headerSize];
    header[1] = 126;

    uint16_t tmp = htons(bufSize);
    header[2] = uint8_t(tmp >> 8);
    header[3] = uint8_t(tmp);
  } else {
    headerSize = 10;
    header = new uint8_t[headerSize];
    header[1] = 127;

    uint64_t tmp = htobe64(bufSize);
    for(int i=0;i<8;i++)
      header[2+i] = uint8_t(tmp >> (56-8*i));
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

