CXX = g++
CXX_FLAGS = -Wall -Wextra -std=c++11
LFLAGS =

WEBSOCKET_FILES = main websocket
WEBSOCKET_OBJS = $(WEBSOCKET_FILES:%=obj/%.o)

.PHONY: all clean

all: $(WEBSOCKET_OBJS)
	$(CXX) $(LFLAGS) $(WEBSOCKET_OBJS) -o bin/server

$(WEBSOCKET_OBJS): obj/%.o: src/%.cpp
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

clean: 
	rm obj/*
