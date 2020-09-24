server: server.o common.o
	cc $^ -libverbs -o $@

client: client.o common.o
	cc $^ -libverbs -o $@

server.o: server.c common.h
	cc -c server.c

client.o: client.c common.h
	cc -c client.c

common.o: common.c 
	cc -c common.c

clean:
	rm common.o client.o client