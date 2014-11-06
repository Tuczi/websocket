websocket
=========
##About
Server socket implementation of Websocket protocol (RFC6455) in C++.
Single class Websocket wraps socket descriptor and provides methods 
to init conneciton (do heandshake) and simple read/write to socket.
It support sending/receiving partial message/frame (small buffer).
Library is using lowlevel/system(Linux) and openssl functions.

##Sample aps
See sample apps:
* chat (text data)
* image sending
* video streaming (TODO)

###TODO 
see TODO.md
