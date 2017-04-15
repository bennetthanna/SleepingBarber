all: sleeping_barber.c
	gcc -Wall -o hw2 sleeping_barber.c -pthread -std=gnu99

clean:
	$(RM) hw2