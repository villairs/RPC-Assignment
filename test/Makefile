all: client1 client2 server

client1: client1.o
	g++ -L. client1.o -lrpc -o client1

client2: client2.o
	g++ -L. client2.o -lrpc -o client2

server: server_functions.o server_function_skels.o server.o
	g++ -L. server_functions.o server_function_skels.o server.o -lrpc -o server

.PHONY: clean

clean:
	rm -f *.o client1 client2 server
