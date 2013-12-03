#CFLAGS=-Wall -Wextra -g

CFLAGS=-Wall -Wextra -DREADLINE -lreadline

all: thymioshell thymiotest

####################################
thymioshell: thymioshell.c thymio.c
	$(CC) $(CFLAGS) -o $@ $^

####################################
clean: 
	rm -fr thymiotest thymioshell thymiotest.dSYM thymioshell.dSYM
