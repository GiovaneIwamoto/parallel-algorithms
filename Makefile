CC = mpicc
CFLAGS = -g -Wall
TARGET = odd-even
SRC = odd-even.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
