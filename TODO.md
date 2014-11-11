TODO list:
* Heandshake
  * Respect statuses
  * Sec-WebSocket-Version - multiple tags, correct negotiation (RFC6455 sectoin 4.4)
* Sample apps
  * Simple video stream example
* Efficient & features
  * Avoid multiple buffor copy (write_header)
  * Methods to read/write:
    * operator >>
    * operatore <<
  * Serialize + deserialize (especialy std::string)
* Functionality
  * Fragmentation - RFC6455 section 5.4
  * Control Frames - RFC5455 section 5.5 (close, ping, pong)

