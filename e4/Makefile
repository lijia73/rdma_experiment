testBW: bw_test.o common.o
	cc -o $@ bw_test.o common.o -libverbs -lpthread

bw_test.o: bw_test.c common.h
	cc -c bw_test.c

common.o: common.c common.h
	cc -c common.c

clean:
	rm *.o testBW