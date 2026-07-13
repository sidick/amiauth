# AmiAuth build.
#
#   make test    build and run the host-side vector/unit tests (default)
#   make cli     build the CLI natively (local dev convenience)
#   make diff    build and run the OpenSSL differential fuzz harness (opt-in)
#   make m68k    cross-build the Amiga CLI binary (needs amiga-gcc)
#   make clean
#
# The core is portable C, so `test` and `cli` build with any host compiler.
# `diff` additionally needs OpenSSL (libcrypto) as a host test dependency.

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
DIFF_SRCS := tests/diff/diff_main.c

# OpenSSL flags for the differential harness (pkg-config, with a plain fallback).
OPENSSL_CFLAGS ?= $(shell pkg-config --cflags libcrypto 2>/dev/null)
OPENSSL_LIBS   ?= $(shell pkg-config --libs libcrypto 2>/dev/null || echo -lcrypto)

# Iterations/primitive for `make diff` (override: make diff DIFF_ITERS=20000).
DIFF_ITERS ?= 5000

BUILD := build

.PHONY: all test cli diff m68k clean

all: test cli

# --- Host: unit / RFC-vector tests ---
test: $(BUILD)/run-tests
	VAULT_TEST_FILE=$(BUILD)/amiauth-test.vault $(BUILD)/run-tests

$(BUILD)/run-tests: $(CORE_SRCS) $(TEST_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) -Itests $(CORE_SRCS) $(TEST_SRCS) -o $@

# --- Host: native CLI (for local development) ---
cli: $(BUILD)/amiauth

$(BUILD)/amiauth: $(CORE_SRCS) $(CLI_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) $(CORE_SRCS) $(CLI_SRCS) -o $@

# --- Host: differential fuzz harness vs OpenSSL (opt-in; needs libcrypto) ---
diff: $(BUILD)/run-diff
	$(BUILD)/run-diff $(DIFF_ITERS)

$(BUILD)/run-diff: $(CORE_SRCS) $(DIFF_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) $(OPENSSL_CFLAGS) $(CORE_SRCS) $(DIFF_SRCS) \
		$(OPENSSL_LIBS) -o $@

# --- m68k: Amiga CLI binary ---
m68k: | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(CORE_SRCS) $(CLI_SRCS) -o $(BUILD)/AmiAuth

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
