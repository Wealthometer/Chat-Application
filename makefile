CC = g++
CFLAGS = -std=c++11 -pthread

all: server client

server: server.cpp
	$(CC) $(CFLAGS) server.cpp -o server

client: client.cpp
	$(CC) $(CFLAGS) client.cpp -o client

clean:
	rm -f server client

run_server: server
	./server

run_client: client
	./client