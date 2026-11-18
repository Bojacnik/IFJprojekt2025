# Implementace překladače imperativního jazyka IFJ25
# Autoři: TODO (login, jméno)
#
# Repository layout: sources in src/, headers in include/, objects in build/.
# The submission archive must be flat (no directories), so the Makefile also
# builds when all .c/.h files sit next to it (see `make pack`).

CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -pedantic -O2
CPPFLAGS = -Iinclude -I. -MMD -MP
LDLIBS   = -lm

TARGET    = ifj25
SRC_DIR   = src
INC_DIR   = include
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard *.c)
HDRS = $(wildcard $(INC_DIR)/*.h) $(wildcard *.h)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))
vpath %.c $(SRC_DIR) .

# Submission archive: make pack LOGIN=xlogin99
LOGIN ?= xlogin99
PACK   = $(SRCS) $(HDRS) Makefile $(wildcard rozdeleni dokumentace.pdf)

.PHONY: all test pack clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

test: $(TARGET)
	./tests/run.sh ./$(TARGET)

# Flat archive: the assignment forbids directories inside it.
pack:
	tar -czf $(LOGIN).tgz --transform='s|.*/||' $(PACK)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) *.tgz

-include $(OBJS:.o=.d)
