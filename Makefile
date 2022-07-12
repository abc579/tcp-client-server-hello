CXX=g++
CXXFLAGS=-std=c++17 -g3 -Wall -Werror -Wextra -Wpedantic -fsanitize=address

all: server client

server:
	$(CXX) $(CXXFLAGS) server.cpp -o server.o

client:
	$(CXX) $(CXXFLAGS) client.cpp -o client.o

clean:
	rm *.o