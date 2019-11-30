all: apsudo apsudo-server

apsudo:
	gcc -o apsudo client.c
apsudo-server:
	gcc server.c `pkg-config --cflags --libs gmodule-2.0` -o apsudo-server
clean:
	rm apsudo apsudo-server
