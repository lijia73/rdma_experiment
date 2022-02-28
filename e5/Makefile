serverb: serverb.o common.o
	cc $^ -libverbs -o $@ -g

clienta: clienta.o common.o
	cc $^ -libverbs -o $@ -g

clientc: clientc.o common.o
	cc $^ -libverbs -o $@ -g

serverb.o: serverb.c common.h
	cc -c serverb.c

clienta.o: clienta.c common.h
	cc -c clienta.c

clientc.o: clientc.c common.h
	cc -c clientc.c

common.o: common.c 
	cc -c common.c

clean:
	rm common.o clienta.o clientc.o clienta clientc serverb.o serverb