all: clevo-fan-control

clevo-fan-control: main.c
	$(CC) -o clevo-fan-control main.c

clean:
	$(RM) main.o clevo-fan-control
