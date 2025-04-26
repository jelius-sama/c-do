CC = clang
CFLAGS = -Wall -Wextra
LDFLAGS = -lsqlite3
SRC := $(shell find src -name '*.c')
OUT = bin/c-do

build: CFLAGS += -DDEBUG
build: all

release: all

all:
	mkdir -p bin db
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

clean:
	rm -rf bin db
