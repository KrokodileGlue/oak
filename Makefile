CC ?= gcc
LD = $(CC)

CPPFLAGS += -MMD -MP
CFLAGS += -std=c11 -Wall -Wextra -pedantic-errors -Iinclude
CFLAGS += -fuse-ld=gold -flto -fuse-linker-plugin
CFLAGS += -Wno-missing-field-initializers -Wstrict-overflow
CFLAGS += -Wunused -Wno-implicit-fallthrough
LDFLAGS += -fuse-ld=gold -flto -fuse-linker-plugin
LDFLAGS += -Wl,-O2,-z,relro,-z,now,--sort-common,--as-needed

TARGET := oak
LIBS := -lm
SRC := $(wildcard src/*.c)
HDR := $(wildcard include/*.h)
OBJ := $(SRC:.c=.o) $(TARGET).o
DEP := $(OBJ:.o=.d) $(TARGET).d

all: CFLAGS += -O2
all:
	@$(MAKE) $(TARGET) CFLAGS="$(CFLAGS)"

debug: CFLAGS += -g -Og
debug: CFLAGS += -Wfloat-equal -Wshadow
debug: CFLAGS += -fno-builtin -fno-common -fno-inline
debug:
	@$(MAKE) $(TARGET) CFLAGS="$(CFLAGS)"

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@
%.o %.d: %.c $(HDR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
clean:
	$(RM) $(OBJ) $(DEP) $(TARGET)

-include $(DEP)
.PHONY: all debug clean $(DEP) $(HDR)
