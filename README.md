websocket
=========
##About
Implementation of Websocket protocol (RFC6455) in C++ (Server-side socket).
Single class Websocket wraps socket descriptor and provides methods 
to init conneciton (do heandshake) and simple read/write to socket.
It supports sending/receiving partial message/frame (small buffer) and
full message/frame.
Library uses lowlevel/system(Linux) and openssl functions.

##Sample apps
See sample apps:
* chat (text data)
* image sending
* video streaming (TODO)

###TODO 
see TODO.md
