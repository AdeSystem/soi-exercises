TARGET = main

CXX = g++
CXXFLAGS = -pthread -Wall -Wextra -std=c++20

SRC = main.cpp
HEADERS = monitor.h

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)