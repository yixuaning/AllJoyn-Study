ALLJOYN_DIST := /home/yasin/IOT/AllJoyn/alljoyn-15.04.00-src/build/linux/x86_64/debug/dist/cpp

OBJ_DIR := obj

BIN_DIR := bin

ALLJOYN_LIB := $(ALLJOYN_DIST)/lib/liballjoyn.a

CXXFLAGS = -Wall -pipe -std=c++98 -fno-rtti -fno-exceptions -Wno-long-long -Wno-deprecated -g -DQCC_OS_LINUX -DQCC_OS_GROUP_POSIX -DQCC_CPU_X86

LIBS = -lstdc++ -lcrypto -lpthread -lrt

.PHONY: default clean

default: all

all: basic_service

#basic_client: basic_client.o $(ALLJOYN_LIB)
#	mkdir -p $(BIN_DIR)
#	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/basic_client.o $(ALLJOYN_LIB) $(LIBS)

#basic_client.o: basic_client.cc $(ALLJOYN_LIB)
#	mkdir -p $(OBJ_DIR)
#	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ basic_client.cc

basic_service: basic_service.o $(ALLJOYN_LIB)
	mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/$@ $(OBJ_DIR)/basic_service.o $(ALLJOYN_LIB) $(LIBS)

basic_service.o: basic_service.cc $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ basic_service.cc

clean: clean_basic_service
	rmdir $(OBJ_DIR)
	rmdir $(BIN_DIR)

#clean_basic_client:
#	rm -f $(OBJ_DIR)/basic_client.o $(BIN_DIR)/basic_client

clean_basic_service:
	rm -f $(OBJ_DIR)/basic_service.o $(BIN_DIR)/basic_service
