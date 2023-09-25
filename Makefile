BIN=bin
SRC=src

CFLAGS=-std=c99 -Wall -Wextra -Werror
CC=gcc $(CFLAGS)

OBJS=$(patsubst $(SRC)/%.c,$(BIN)/%.o,$(wildcard $(SRC)/*.c))

.PHONY: all clean

all: $(BIN)/compiler

$(BIN)/compiler: $(OBJS)
	$(CC) -o $@ $(OBJS)

$(BIN)/%.o: $(SRC)/%.c
	$(CC) $@ $<

testCppcheck:
	cppcheck --std=c11 --enable=all --suppress=missingIncludeSystem $(SRC)

testClang:
	clang-format --dry-run -Werror $(SRC)/*.c

clean:
	rm -rf $(BIN)/*
