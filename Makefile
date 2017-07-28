src = $(wildcard src/*.c)
src += $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS = -std=c11 -Wall -Wextra -pedantic -Wunused -I include/
LDFLAGS += -lm

debug: CFLAGS += -g
release: CFLAGS += -O2

all: $(obj)
	$(CC) -o oak $^ $(LDFLAGS)

debug: all
release: all

.PHONY: clean
clean:
	rm -f $(obj) oak
