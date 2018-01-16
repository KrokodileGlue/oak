CPPFLAGS := -MMD -MP
CFLAGS += -O2 -Iinclude -std=c11 -Wall -Wextra -pedantic
CFLAGS += -Wunused -Wno-implicit-fallthrough
LDFLAGS += -Wl,--as-needed,-O2,-z,relro,-z,now
LDLIBS += -lm

TARGET := oak
SRC := $(wildcard src/*.c)
OBJ = $(SRC:.c=.o) $(TARGET).o
DEP = $(OBJ:.o=.d)

all: $(TARGET)

debug: all
debug: CFLAGS += -g -Og
debug: LDFLAGS += -g -Og

$(TARGET): $(OBJ)
	$(LINK.o) $^ $(LDLIBS) -o $@

clean: $(TARGET)
	$(RM) $(OBJ) $(DEP) $(TARGET)

-include $(DEP)

.PHONY: all debug clean
