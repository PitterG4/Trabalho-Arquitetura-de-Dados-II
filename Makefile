CC = gcc
CFLAGS = -Wall -O2
TARGET = trabalho

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

data:
	python download.py

clean:
	rm -f $(TARGET) $(TARGET).exe temp.bin
