-include ../config.mk

MUSL = /Users/dspace/musl
LIBC_LIBS = $(wildcard /usr/local/musl/lib*)
LIBC_INCS = $(wildcard /usr/local/musl/include/**/*)
LIBC_DEPS = $(LIBC_LIBS) $(LIBC_INCS)

OBJ = ../obj/dyn

USR_CC := $(MUSL)/bin/musl-gcc
CFLAGS = -std=gnu99 -O3 -z max-page-size=4096 -fpie

BIN := $(OBJ)/bin
SRC := src

USER_DIRS := $(shell find $(SRC) -maxdepth 1 -mindepth 1 -type d)
USER_BINS := $(USER_DIRS:$(SRC)/%=$(BIN)/%)

all:
	mkdir -p $(OBJ)
	$(MAKE) $(USER_BINS)

$(OBJ)/%.c.o: %.c $(LIBC_DEPS)
	mkdir -p $(dir $@)
	$(USR_CC) $(CFLAGS) -c -o $@ $<

# Same as `bin/NAME: $(OBJ)/NAME/*.c.o $(LIBC_A)`
.SECONDEXPANSION:
$(BIN)/%: $$(addsuffix .o, $$(addprefix $(OBJ)/, $$(shell find $(SRC)/% -name "*.c")))
	mkdir -p $(dir $@)
	$(USR_CC) $(CFLAGS) -pie -o $@ $^
	$(OBJDUMP) -S -d $@ > $(OBJ)/src/$(notdir $@)/$(notdir $@).asm
	$(OBJDUMP) -x $@ > $(OBJ)/src/$(notdir $@)/$(notdir $@).hdr
	$(STRIP) --strip-debug $@

clean:
	rm -rf $(OBJ) $(BIN)

.PHONY: all clean
