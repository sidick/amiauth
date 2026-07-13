# AmiAuth build.
#
#   make test    build and run the host-side vector/unit tests (default)
#   make cli     build the CLI natively (local dev convenience)
#   make m68k    cross-build the Amiga CLI binary (needs amiga-gcc)
#   make clean
#
# The core is portable C, so `test` and `cli` build with any host compiler.

# --- Host toolchain (tests + native CLI) ---
CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
CObjINC := -Isrc/core

# --- m68k cross toolchain (Amiga build) ---
M68K_CC     ?= m68k-amigaos-gcc
M68K_CFLAGS ?= -std=c99 -O2 -Wall -m68000 -noixemul $(CObjINC)

CORE_SRCS := $(wildcard src/core/*.c)
TEST_SRCS := $(wildcard tests/*.c)
CLI_SRCS  := src/cli/main.c

BUILD := build

.PHONY: all test cli m68k clean

all: test cli

# --- Host: unit / RFC-vector tests ---
test: $(BUILD)/run-tests
	$(BUILD)/run-tests

$(BUILD)/run-tests: $(CORE_SRCS) $(TEST_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) -Itests $(CORE_SRCS) $(TEST_SRCS) -o $@

# --- Host: native CLI (for local development) ---
cli: $(BUILD)/amiauth

$(BUILD)/amiauth: $(CORE_SRCS) $(CLI_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) $(CORE_SRCS) $(CLI_SRCS) -o $@

# --- m68k: Amiga CLI binary ---
m68k: | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(CORE_SRCS) $(CLI_SRCS) -o $(BUILD)/AmiAuth

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
