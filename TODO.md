TODO list:
* Sample apps
* Video stream example
* Avoid multiple buffor copy (write_header)
* Handshake out of constructor
* Client Heandshake behavior:
  * origin accepting?
  * accpet only version 13 (repeating heandshake)
  * subprotocol
  * extensions ?
* Server Heandshake responce:
  * Sec-Websocket-Protocol?
  * Sec-Websocket-Extensions
* Data framing:
  * rsv1,2,3 - extension
  * opcode - ping and pong 
  * extension data !=0
* Fragmentation - RFC6455 section 5.4
* Control Frames - RFC5455 section 5.5

