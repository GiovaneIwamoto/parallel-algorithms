CC = mpicc
CFLAGS = -g -Wall
TARGETS = odd-even quicksort-parallel
SOURCES = odd-even.c quicksort-parallel.c

all: $(TARGETS)

odd-even: odd-even.c
	$(CC) $(CFLAGS) -o $@ $<

quicksort-parallel: quicksort-parallel.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
