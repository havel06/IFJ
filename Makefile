BIN=bin
SRC=src

CFLAGS=-std=c99 -Wall -Wextra -Werror -std=c11
CC=gcc $(CFLAGS)

OBJS=$(patsubst $(SRC)/%.c,$(BIN)/%.o,$(wildcard $(SRC)/*.c))

.PHONY: all clean

$(BIN)/compiler: $(OBJS)
	$(CC) -o $@ $^

$(BIN)/%.o: $(SRC)/%.c
	$(CC) -o $@ -c $<

format:
	clang-format -i $(SRC)/*

testCppcheck:
	cppcheck --std=c11 --enable=all --suppress=missingIncludeSystem --error-exitcode=1 $(SRC)

testClang:
	clang-format --dry-run -Werror $(SRC)/*

clean:
	rm -rf $(BIN)/*
