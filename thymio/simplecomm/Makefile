CFLAGS=-Wall

####################################
main: main.o thymio.o
	$(CC) -o $@ main.o thymio.o	

####################################
test: main_test.o thymio.o
	$(CC) -o $@ main_test.o thymio.o

####################################
main.o: main.c thymio.o
	$(CC) $(CFLAGS) -o $@ -c $<	

main_test.o: main_test.c thymio.o
	$(CC) $(CFLAGS) -o $@ -c $<	

thymio.o: thymio.c thymio.h
	$(CC) $(CFLAGS) -o $@ -c $<

####################################
clean: 
	rm -f *.o test
