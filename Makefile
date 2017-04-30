src = $(wildcard src/*.c)
src += $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS = -g -std=c11 -Wall -Wextra -pedantic -Wunused -I include/

oak: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) oak
