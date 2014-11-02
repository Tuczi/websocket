#ifndef TUCZI_WEBSOCKET_HPP
#define TUCZI_WEBSOCKET_HPP

#define _BSD_SOURCE
#include <endian.h>


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

/**
 * Websocket protocol (RFC6435) server socket implementation.
 * Supported versions: 13.
 *
 * It wraps socket descriptor and provides methods to read/send data.
 * Supports partially data sending and receiving (low buffers).
 *
 * No threads are used to send/receive data! (You can use non-blocking IO mechanisms or threads).
 */
class Websocket {
  struct ReadCtx {
    bool fin = false;
    bool mask = false;
    uint8_t maskingKey[4];
    uint8_t maskingKeyIter=0;
    uint64_t frameSize = 0;
  };

  private:
    const int descriptor;
    uint64_t writeFrameSize;
    ReadCtx readCtx;

    static const size_t BUF_SIZE = 1024;
    static const size_t MAX_HEADER_SIZE = 10;

    std::string parseHeandshake(std::string& input);
    void heandshakeResponce(std::string& keyResponce);
    std::string encodeBase64(unsigned char input[SHA_DIGEST_LENGTH]);

    size_t parseFrame(uint8_t * buffer, size_t bufferSize);
    void frameHeader(size_t dataSize, uint8_t (&header)[MAX_HEADER_SIZE], size_t& headerSize);

  public:
    Websocket(int descriptor_): descriptor(descriptor_), writeFrameSize(0), readCtx() {
      std::string buffer(BUF_SIZE, 0);
      ::read(descriptor, (void*) buffer.c_str(), BUF_SIZE);
      std::string key = parseHeandshake(buffer);
      heandshakeResponce(key);
    }

  /**
   * Read from websocket. This method can be called multiple times to
   *
   * @return true if have to be called again (to receive next part of data). false otherwise.
   */
  bool read(uint8_t* buffer, size_t bufferSize, size_t& dataRead);

  /**
   * Send data into client. May be called multiple times.
   * writeHeader method have to be called before first call of this metod!
   *
   * @see writeHeader
   *
   * @return true if have to be called again (to send next part of data). false otherwise
   */
  bool write(uint8_t* buffer, size_t bufferSize, size_t& dataWritten);

  /**
   * Start sending frame (or frames to client).
   * Send websocketHeader. After call this method
   * method write can be called.
   *
   * @param dataSize size of application data
   * @param dataType type of data to be send (see opcode in RFC6455)
   *  default 1.
   * @return true if correct. false otherwise
   */
  bool writeHeader(size_t dataSize, uint8_t dataType = 1); //TODO #define or const
};
}
#endif

