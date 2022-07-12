CXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Werror -Wextra -Wpedantic -fsanitize=address

all: server client

server:
	$(CXX) $(CXXFLAGS) server.cpp -o server.o

client:
	$(CXX) $(CXXFLAGS) client.cpp -o client.o

clean:
	rm *.o