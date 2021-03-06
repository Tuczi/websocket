#ifndef TUCZI_WEBSOCKET_HPP
#define TUCZI_WEBSOCKET_HPP

#include <endian.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <netdb.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

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
  static const size_t HEANDSHAKE_BUF_SIZE = 1024;
  static const size_t HEANDSHAKE_KEY_SIZE = 100;
  static const size_t MAX_HEADER_SIZE = 10;

  struct ReadCtx {
    bool fin = false;
    bool mask = false;
    uint8_t opcode = 0;
    uint8_t maskingKey[4];
    uint8_t maskingKeyIter=0;
    uint64_t frameSize = 0;
  };
  struct WriteCtx {
    uint64_t frameSize = 0;
  };

  struct HeandshakeCtx {
    std::vector<int> versions;
    //std::vector<char*> protocols; //TODO
    std::string responceKey;//TODO initialize with keysize
  };

  public:
    enum Opcode: uint8_t {
      CONTINUATION = 0,
      TEXT = 1,
      BINARY = 2,
      CONNECTION_CLOSE = 8,
      PING_FRAME = 9,
      PONG_FRAME = 10
    };

    struct Frame {
      struct data_t {//TODO union
        std::string str;
        std::vector<uint8_t> bin;

        data_t() { }
        data_t(std::string& str_): str(str_) { }
        data_t(std::vector<uint8_t>& bin_): bin(bin_) { }
        ~data_t() { }
      };

      bool isText;
      data_t data;

      Frame(std::string& str): isText(true), data(str) { }
      Frame(std::vector<uint8_t>& bin): isText(false), data(bin) { }

      Frame(): isText(false), data() { }
      /*Frame(Frame&& frame): isText(frame.isText) {
        if(frame.isText) data.str=frame.data.str;
        else data.bin=frame.data.bin;
      }*/

      ~Frame() {
        //if(isText) data.str.~basic_string<char>();
        //else data.bin.~vector<uint8_t>();
      }

      /*Frame& operator= (Frame& frame) {
        isText = frame.isText;
        if(isText) data.str = frame.data.str;
        else data.bin = frame.data.bin;
        return *this;
      }*/
    };


  private:
    const int descriptor;
    WriteCtx writeCtx;
    ReadCtx readCtx;

    void parseHeandshake(std::string &buffer, HeandshakeCtx& heandshakeCtx);
    void heandshakeResponce(HeandshakeCtx& heandshakeCtx);
    std::string encodeBase64(unsigned char (&input)[SHA_DIGEST_LENGTH]);

    size_t parseFrame(uint8_t * buffer, size_t bufferSize);
    void frameHeader(size_t dataSize, Opcode opcode, uint8_t (&header)[MAX_HEADER_SIZE], size_t& headerSize);

  /**
   * Read from websocket. This method can be called multiple times to
   *
   * @return true if have to be called again (to receive next part of data). false otherwise.
   */
  bool read_(uint8_t* buffer, size_t bufferSize, size_t& dataRead);

  /**
   * Send data into client. May be called multiple times.
   * writeHeader method have to be called before first call of this metod!
   *
   * @see writeHeader
   *
   * @return true if have to be called again (to send next part of data). false otherwise
   */
  bool write_(uint8_t* buffer, size_t bufferSize, size_t& dataWritten);

  public:
    /**
     * Construct an object. Set memory only. Do not send any data.
     */
    Websocket(int descriptor_): descriptor(descriptor_), writeCtx(), readCtx() { }

    /**
     * Init a connection. Send heandshake.
     *
     * @return true if connection correctly established
     */
    bool init();

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
  bool writeHeader(size_t dataSize, Opcode dataType = Opcode::TEXT); //TODO #define or const

  /**
   * Write buffer whole to socket.
   * bufferSize may be diffrent then headerSize.
   * May be call multiple times.
   * WriteHeader must be called first!
   *
   * @return true if written (whole buffer), false = error
   */
  bool writePart(void* buffer, size_t bufferSize);

  /**
   * Read from websocket to buffer.
   * bufferSize may be diffrent then headerSize.
   * May be called multiple times.
   *
   * @return true if read (whole buffer), false = error
   */
  bool readPart(void* buffer, size_t bufferSize, size_t& dataRead);

  /**
   * Read whole message.
   * Dynamic memory allocation.
   *
   * @param buffer buffer pointer reference
   * @param bufferSize size of readed message(frame)
   *
   * @return true if readed, false otherwise
   */
  bool read(void*& buffer, size_t& bufferSize);

  /**
   * Write whole message (buffer) as frame.
   *
   * @return true if written, false otherwise
   */
  inline bool write(void* buffer, size_t bufferSize, Opcode dataType = Opcode::TEXT) {
    return writeHeader(bufferSize, dataType) && writePart(buffer, bufferSize);
  }

  /**
   * Read whole message (frame).
   */
  Frame read();

  /**
   * Read frame from socket using operator>>
   */
  template <typename T>
  Websocket& operator>>(T& in) {
    size_t tmp;
    readPart((void*)&in, sizeof(T), tmp);
    return *this;
  }

  /**
   * Read from socket to Frame type using operator>>
   */
  Websocket& operator>> (Frame& frame) {
    frame = read();
    return *this;
  }

  /**
   * Write to socket from using operator<<
   */
  template <typename T>
  Websocket& operator<<(T& out) {
    write((void*)&out, sizeof(T), Opcode::BINARY);
    return *this;
  }

  /**
   * Write to socket from Frame using operator>>
   */
  Websocket& operator<<(Frame& frame) {
    if(frame.isText)
      write((void*)frame.data.str.c_str(), frame.data.str.size());
    else
      write((void*)frame.data.bin.data(), frame.data.bin.size(), Opcode::BINARY);
    return *this;
  }

  /**
   * Write to socket from std::string using operator>>
   */
  Websocket& operator<<(::std::string& str) {
    write((void*)str.c_str(), str.size());
    return *this;
  }
};
}
#endif

