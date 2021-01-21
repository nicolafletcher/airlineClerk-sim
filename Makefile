
ACS: ACS.o cQueue.o
	gcc -pthread ACS.o cQueue.o -o ACS

ACS.o: ACS.c customer_info.h
	gcc -c ACS.c

cQueue.o: cQueue.c cQueue.h customer_info.h
	gcc -c cQueue.c

clean:
	rm *.o ACS
