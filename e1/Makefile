server: server.o common.o
	cc $^ -libverbs -o $@ -g

client: client.o common.o
	cc $^ -libverbs -o $@ -g

server.o: server.c common.h
	cc -c server.c

client.o: client.c common.h
	cc -c client.c

common.o: common.c 
	cc -c common.c

clean:
	rm common.o client.o client server.o server