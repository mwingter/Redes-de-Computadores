all:
	mkdir -p bin/
	gcc -Wall -g3 -fsanitize=address -pthread src/server.c -o bin/server
	gcc -Wall -g3 -fsanitize=address -pthread src/client.c -o bin/client 

server:
	./bin/server 1234

client:
	./bin/client 1234
