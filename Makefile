
CC = g++
CFLAGS = -Wall -g
SERVER = server.cpp server_helper.cpp
CLIENT = client.cpp
# Portul pe care asculta serverul (de completat)
PORT = 12345

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

all: build

build: server client

# Compileaza server.cpp
server: server.o server_helper.o
	$(CC) $(CFLAGS) $^ -o $@

server.o: server.cpp
	$(CC) $(CFLAGS) $^ -c

server_helper.o: server_helper.cpp
	$(CC) $(CFLAGS) $^ -c

# Compileaza client.cpp
client: client.o client_helper.o
	$(CC) $(CFLAGS) $^ -o $@

client.o: client.cpp
	$(CC) $(CFLAGS) $^ -c

client_helper.o: client_helper.cpp
	$(CC) $(CFLAGS) $^ -c

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_client:
	./client ${IP_SERVER} ${PORT}

clean:
	rm -f server client
