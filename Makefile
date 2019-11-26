all: apsudo apsudo-server

apsudo:
	gcc -o apsudo client.c
apsudo-server:
	gcc -o apsudo-server server.c
clean:
	rm apsudo apsudo-server
