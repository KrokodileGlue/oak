src = $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS = -std=c11 -Wall -Wextra -pedantic -Wunused

woodwinds: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) woodwinds
