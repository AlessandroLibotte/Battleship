all: server client

server: server_main.c
	gcc server_main.c -o server

client: test_client.c
	gcc test_client.c -o client -pthread
