CC=gcc
CFLAGS=-Wall -Wextra -g

TARGET=main
SRC_FILES=$(wildcard *.c)
OBJ_FILES=$(SRC_FILES:.c=.o)

all: $(TARGET) cleanup

$(TARGET): $(OBJ_FILES)
	$(CC) -o $@ $^

%.o: %.c %.h
	$(CC) -o $@ -c $<

cleanup:
	rm -rf *.o
