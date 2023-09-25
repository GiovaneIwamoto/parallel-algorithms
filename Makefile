CC = mpicc
CFLAGS = -g -Wall
LDFLAGS = -lm  # Adicione a flag -lm aqui
TARGETS = odd-even quicksort-parallel samplesort-introsort samplesort-quicksort samplesort-introsort-binary
SOURCES = odd-even.c quicksort-parallel.c samplesort-introsort.c samplesort-quicksort.c samplesort-introsort-binary.c

all: $(TARGETS)

odd-even: odd-even.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

quicksort-parallel: quicksort-parallel.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

samplesort-introsort: samplesort-introsort.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

samplesort-quicksort: samplesort-quicksort.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

samplesort-introsort-binary: samplesort-introsort-binary.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
