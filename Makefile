NAME := xboxcdtest
BUILD := build

CC := xenon-gcc
OBJCOPY := xenon-objcopy
ELF2XEX ?= elf2xex

SRC := src/main.c src/xboxcd.c src/xboxcd_log.c
OBJ := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

ELF := $(BUILD)/$(NAME).elf
XEX := $(BUILD)/$(NAME).xex

CFLAGS := -O2 -g -Wall -Wextra -DXENON
CFLAGS += -I$(DEVKITXENON)/usr/include

# CORREÇÃO: Adiciona os caminhos corretos para as bibliotecas
LDFLAGS := -T$(DEVKITXENON)/app.lds
LDFLAGS += -L$(DEVKITXENON)/usr/lib
LDFLAGS += -L$(DEVKITXENON)/lib/gcc/xenon/9.2.0
LDFLAGS += -L$(DEVKITXENON)/xenon/lib

# CORREÇÃO: Adiciona -lgcc e -lc (ordem importa!)
LIBS := -lxenon -lm -lgcc -lc

.PHONY: all elf xex clean

all: xex

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

elf: $(ELF)

$(ELF): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

xex: $(XEX)

$(XEX): $(ELF)
	@command -v $(ELF2XEX) >/dev/null 2>&1 || { echo "ERRO: elf2xex não encontrado."; exit 1; }
	$(ELF2XEX) $< $@

clean:
	rm -rf $(BUILD)
