CC=~/Downloads/buildroot-2024.02/output/host/bin/arm-linux-gcc

default: client server

client: 
	gcc client.c -o client.out

server: 
	$(CC) server.c -o server.out
 
clean: 
	rm server.out client.out