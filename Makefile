# Implementace překladače imperativního jazyka IFJ25
# Autoři: TODO (login, jméno)
#
# Works both in the repository (sources in src/) and in the flat submission
# archive (all sources next to this Makefile).

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -pedantic -O2 -Isrc
LDLIBS  = -lm
TARGET  = ifj25

SRCS    = $(wildcard *.c) $(wildcard src/*.c)
HDRS    = $(wildcard *.h) $(wildcard src/*.h)
OBJS    = $(SRCS:.c=.o)

# Submission archive: make pack LOGIN=xlogin99
LOGIN  ?= xlogin99
PACK    = $(SRCS) $(HDRS) Makefile $(wildcard rozdeleni dokumentace.pdf)

.PHONY: all test pack clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	./tests/run.sh ./$(TARGET)

# Flat archive: the assignment forbids directories inside it.
pack:
	tar -czf $(LOGIN).tgz --transform='s|.*/||' $(PACK)

clean:
	rm -f $(TARGET) $(OBJS) *.tgz
