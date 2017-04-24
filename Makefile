src = $(wildcard src/*.c)
obj = $(src:.c=.o)

CFLAGS = -g -std=c11 -Wall -Wextra -pedantic -Wunused -I include/

woodwinds: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) woodwinds
