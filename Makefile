# Implementace překladače imperativního jazyka IFJ25
# Autoři: TODO (login, jméno)

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -pedantic -O2
TARGET  = ifj25

SRCS    = main.c $(wildcard src/*.c)
OBJS    = $(SRCS:.c=.o)

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	./tests/run.sh ./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
