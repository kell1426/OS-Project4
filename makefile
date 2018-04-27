CC=gcc
CFLAGS=-std=c99
DBFLAGS=-g

make: Server Client

Server: server.c
	$(CC) $(CFLAGS) -pthread -o server server.c

Client: client.c
	$(CC) $(CFLAGS) -pthread -o client client.c

debug: Server_debug Client_debug

Server_debug:
	$(CC) $(CFLAGS) $(DBFLAGS) -pthread -o server server.c

Client_debug:
	$(CC) $(CFLAGS) $(DBFLAGS) -pthread -o client client.c

clean:
	rm server client
