CFLAGS=-Wall

test: main_test.o thymio.o
	$(CC) -o test main_test.o thymio.o

main_test.o: main_test.c
	$(CC) $(CFLAGS) -o $@ -c $<	
thymio.o: thymio.c thymio.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean: 
	rm -f *.o test
