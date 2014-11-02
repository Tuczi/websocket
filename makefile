CXX = g++
CXX_FLAGS = -Wall -Wextra -std=c++11
LFLAGS = -lcrypto

WEBSOCKET_FILES = websocket
WEBSOCKET_OBJS = $(WEBSOCKET_FILES:%=obj/%.o)

TEST_I_FILES = main websocket exampleServer
TEST_I_OBJS = $(TEST_I_FILES:%=obj/test/server/%.o)

TEST_T_FILES = main websocket exampleServerImage
TEST_T_OBJS = $(TEST_T_FILES:%=obj/test/server/%.o)

.PHONY: all clean test-text test-image

all: $(WEBSOCKET_OBJS)
	$(CXX) $(LFLAGS) $(WEBSOCKET_OBJS) -o bin/server

test-text: $(WEBSOCKET_OBJS) $(TEST_I_OBJS)	
	$(CXX) $(LFLAGS) $(WEBSOCKET_OBJS) $(TEST_I_OBJS) -o bin/test-server

test-image: $(WEBSOCKET_OBJS) $(TEST_T_OBJS)	
	$(CXX) $(LFLAGS) $(WEBSOCKET_OBJS) $(TEST_T_OBJS) -o bin/test-server

$(WEBSOCKET_OBJS): obj/%.o: src/%.cpp
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

$(TEST_I_OBJS): obj/test/server%.o: test/server/%.cpp
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

$(TEST_T_OBJS): obj/test/server%.o: test/server/%.cpp
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

clean: 
	rm -f obj/*.o
	rm -f obj/test/server/*.o
