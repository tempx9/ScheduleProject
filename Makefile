CXX := g++
CXX_FLAGS := -Wall -Wextra -std=c++17 -ggdb

BIN := bin
SRC := src/Schedule.cpp
INCLUDE := -I/opt/homebrew/include \
           -I/opt/homebrew/include/bsoncxx/v_noabi \
           -I/opt/homebrew/include/mongocxx/v_noabi \
           -I/opt/homebrew/include/bsoncxx/v_noabi/bsoncxx/third_party/mnmlstc

LIBRARIES := -L/opt/homebrew/lib -lboost_system -lpthread -lcrypto -lssl -lmongocxx -lbsoncxx -lcpprest -ljwt
EXECUTABLE := main

all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)
	$(CXX) $(CXX_FLAGS) $(INCLUDE) $^ -o $@ $(LIBRARIES)

clean:
	-rm -rf $(BIN)/*