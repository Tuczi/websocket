CXX = g++
CXX_FLAGS = -Wall -Wextra -std=c++11
LFLAGS = -lcrypto
OPENCV_L_FLAGS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

WEBSOCKET_FILES = websocket
WEBSOCKET_OBJS = $(WEBSOCKET_FILES:%=obj/%.o)

TEST_FILES = main websocket exampleServer
TEST_OBJS = $(TEST_FILES:%=obj/test/server/%.o)

.PHONY: all clean test

all: $(WEBSOCKET_OBJS)
	$(CXX) $(LFLAGS) $(WEBSOCKET_OBJS) -o bin/server

test: $(WEBSOCKET_OBJS) $(TEST_OBJS)	
	$(CXX) $(LFLAGS) $(OPENCV_L_FLAGS) $(WEBSOCKET_OBJS) $(TEST_OBJS) -o bin/test-server

$(WEBSOCKET_OBJS): obj/%.o: src/%.cpp
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

$(TEST_OBJS): obj/test/server%.o: test/server/%.cpp
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

clean: 
	rm -f obj/*.o
	rm -f obj/test/server/*.o
