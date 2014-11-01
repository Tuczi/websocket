#ifndef TUCZI_WEBSOCKET_HPP
#define TUCZI_WEBSOCKET_HPP

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include <iostream>
#include <string>
#include <sstream>

#define LINE_END "\n\r"

namespace tuczi {
class Websocket {
  private:
    const int descriptor;

    static const size_t BUF_SIZE = 1024;
    
    std::string parseHeandshake(std::string& input);
    void heandshakeResponce(std::string& keyResponce);
    std::string encodeBase64(unsigned char input[SHA_DIGEST_LENGTH]);
    
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
#endif
