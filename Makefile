# AmiAuth build.
#
#   make test    build and run the host-side vector/unit tests (default)
#   make cli     build the CLI natively (local dev convenience)
#   make smoke   build the CLI and run the end-to-end CLI smoke test
#   make diff        build and run the OpenSSL differential fuzz harness (opt-in)
#   make m68k        cross-build the Amiga binary (needs amiga-gcc on PATH)
#   make m68k-docker cross-build inside the CI container (no local toolchain)
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
M68K_CFLAGS ?= -std=c99 -O2 -Wall -m68000 -noixemul $(CObjINC) -Isrc/amiga

# Containerised cross-build: same image as CI, so local m68k builds match.
DOCKER          ?= docker
AMIGA_GCC_IMAGE ?= stefanreinauer/amiga-gcc:latest

CORE_SRCS  := $(wildcard src/core/*.c)
TEST_SRCS  := $(wildcard tests/*.c)
CLI_SRCS   := src/cli/main.c

# --- Vendored QR decoder (quirc, ISC) + our portable wrapper -----------------
# quirc is third-party, so it's compiled to objects with warnings OFF (it isn't
# our lint to enforce); our qr.c wrapper builds under the normal warning set.
# QUIRC_FLOAT_TYPE=float matches the m68k build (no FPU), so the host test
# exercises the exact float path that ships on the Amiga.
QUIRC_SRCS      := src/qr/quirc.c src/qr/decode.c src/qr/identify.c src/qr/version_db.c
QR_WRAP         := src/qr/qr.c
# QUIRC_FLOAT_TYPE=float + QUIRC_USE_TGMATH: single-precision soft-float on the
# FPU-less 68000 (quirc's identify geometry is float-heavy; the pair is what its
# docs recommend, and avoids slow double soft-float via rintf/sqrtf/fabsf).
QR_CPPFLAGS     := -Isrc/qr -DQUIRC_FLOAT_TYPE=float -DQUIRC_USE_TGMATH
DIFF_SRCS  := tests/diff/diff_main.c
# AmigaOS-only front-end glue (bsdsocket SNTP, ...); m68k build only. qrimage.c
# is GUI-only (datatypes.library) so it's excluded here and added to GUI_SRCS.
AMIGA_SRCS := $(filter-out src/amiga/qrimage.c,$(wildcard src/amiga/*.c))

# OpenSSL flags for the differential harness (pkg-config, with a plain fallback).
OPENSSL_CFLAGS ?= $(shell pkg-config --cflags libcrypto 2>/dev/null)
OPENSSL_LIBS   ?= $(shell pkg-config --libs libcrypto 2>/dev/null || echo -lcrypto)

# Iterations/primitive for `make diff` (override: make diff DIFF_ITERS=20000).
DIFF_ITERS ?= 5000

BUILD := build

# Object paths for the vendored quirc decoder (needs $(BUILD), defined above).
QUIRC_HOST_OBJS := $(patsubst src/qr/%.c,$(BUILD)/qr-host/%.o,$(QUIRC_SRCS))
QUIRC_M68K_OBJS := $(patsubst src/qr/%.c,$(BUILD)/qr-m68k/%.o,$(QUIRC_SRCS))

.PHONY: all test cli smoke diff m68k m68k-docker gui gui-docker gui-smoke qr-onhw qr-onhw-docker qr-onhw-smoke serialtest-m68k serialtest-m68k-docker copperline-smoke pbkdf2-bench clean

all: test cli

# --- Host: unit / RFC-vector tests ---
test: $(BUILD)/run-tests
	VAULT_TEST_FILE=$(BUILD)/amiauth-test.vault \
		AMIAUTH_PREFS_DIR=$(BUILD)/prefs-test $(BUILD)/run-tests

$(BUILD)/run-tests: $(CORE_SRCS) $(TEST_SRCS) $(QR_WRAP) $(QUIRC_HOST_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) $(QR_CPPFLAGS) -Itests \
		$(CORE_SRCS) $(TEST_SRCS) $(QR_WRAP) $(QUIRC_HOST_OBJS) -o $@

# Vendored quirc objects — host toolchain, warnings suppressed (third-party).
$(BUILD)/qr-host/%.o: src/qr/%.c | $(BUILD)
	@mkdir -p $(BUILD)/qr-host
	$(CC) -std=c99 -O2 -w $(QR_CPPFLAGS) -c $< -o $@

# --- Host: native CLI (for local development) ---
# Named distinctly from the m68k 'AmiAuth' binary so the two don't collide on a
# case-insensitive filesystem (macOS).
cli: $(BUILD)/amiauth-host

$(BUILD)/amiauth-host: $(CORE_SRCS) $(CLI_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) $(CORE_SRCS) $(CLI_SRCS) -o $@

# --- Host: end-to-end CLI smoke test (always-unlocked round trip) ---
smoke: $(BUILD)/amiauth-host
	AMIAUTH_BIN=$(BUILD)/amiauth-host sh tests/cli/smoke.sh

# --- Host: differential fuzz harness vs OpenSSL (opt-in; needs libcrypto) ---
diff: $(BUILD)/run-diff
	$(BUILD)/run-diff $(DIFF_ITERS)

$(BUILD)/run-diff: $(CORE_SRCS) $(DIFF_SRCS) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) $(OPENSSL_CFLAGS) $(CORE_SRCS) $(DIFF_SRCS) \
		$(OPENSSL_LIBS) -o $@

# --- m68k: Amiga CLI binary (amiga-gcc on PATH) ---
m68k: | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(CORE_SRCS) $(AMIGA_SRCS) $(CLI_SRCS) -o $(BUILD)/AmiAuth

# --- m68k: ReAction GUI binary (Amiga only; needs intuition + ReAction classes) ---
# Includes the QR decoder: qrimage.c (datatypes glue) + our qr.c wrapper + the
# vendored quirc objects (built -w for m68k). QUIRC_FLOAT_TYPE=float: no FPU.
GUI_SRCS := src/gui/main.c src/amiga/qrimage.c

# Vendored quirc objects — m68k toolchain, warnings suppressed (third-party).
$(BUILD)/qr-m68k/%.o: src/qr/%.c | $(BUILD)
	@mkdir -p $(BUILD)/qr-m68k
	$(M68K_CC) -std=c99 -O2 -m68000 -noixemul -w $(QR_CPPFLAGS) -c $< -o $@

gui: $(QUIRC_M68K_OBJS) | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(QR_CPPFLAGS) $(CORE_SRCS) $(AMIGA_SRCS) $(GUI_SRCS) \
		$(QR_WRAP) $(QUIRC_M68K_OBJS) -lm -lamiga -o $(BUILD)/AmiAuthGUI

gui-docker:
	$(DOCKER) run --rm --platform linux/amd64 -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make gui'

# --- Headless GUI smoke test: boot WB 3.2 under Copperline, render AmiAuthGUI --
# Boots an A1200/OS 3.2 under native Copperline, auto-launches AmiAuthGUI, and
# asserts the ReAction window rendered (screenshot in build/gui-smoke/). No VNC
# or clicking. Needs `copperline` on PATH, the paths in tests/gui/.env, and a
# prebuilt build/AmiAuthGUI (make gui-docker). See tests/gui/gui-smoke.sh.
gui-smoke: $(BUILD)/amiauth-host
	sh tests/gui/gui-smoke.sh

# --- Headless on-target QR pipeline test: boot WB 3.2, load a staged QR PNG via
# datatypes.library (src/amiga/qrimage.c) + decode it (src/qr), emit the URI over
# serial. Validates the datatypes glue on real picture.datatype. Build the v39
# fallback with:  make qr-onhw-docker QR_ONHW_DEFS=-DQRIMAGE_FORCE_V39
QR_ONHW_DEFS ?=
QR_ONHW_SRCS := tests/gui/qr_onhw.c src/amiga/qrimage.c $(QR_WRAP)

qr-onhw: $(QUIRC_M68K_OBJS) | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(QR_CPPFLAGS) $(QR_ONHW_DEFS) $(QR_ONHW_SRCS) \
		$(QUIRC_M68K_OBJS) -lm -lamiga -o $(BUILD)/qr-onhw

qr-onhw-docker:
	$(DOCKER) run --rm --platform linux/amd64 -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make qr-onhw QR_ONHW_DEFS=$(QR_ONHW_DEFS)'

qr-onhw-smoke:
	sh tests/gui/qr-onhw.sh

# --- m68k via the CI container: no local toolchain needed, matches CI exactly ---
m68k-docker:
	$(DOCKER) run --rm --platform linux/amd64 -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make m68k'

# --- Copperline: headless on-target core smoke test (spike) ------------------
# Boots a stock A500/68000 under Copperline, runs the RFC 4226 HOTP vectors on
# real m68k, and checks the codes it emits over serial. See tests/copperline.
# OTP core chain (hotp_sha1 -> hmac -> sha1) + the DRBG; no vault/prefs/front-end.
SERIALTEST_SRCS := src/core/otp.c src/core/hmac.c src/core/sha1.c src/core/drbg.c \
                   tests/copperline/serialtest.c

serialtest-m68k: | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(SERIALTEST_SRCS) -o $(BUILD)/serialtest

serialtest-m68k-docker:
	$(DOCKER) run --rm --platform linux/amd64 -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make serialtest-m68k'

copperline-smoke: serialtest-m68k-docker
	sh tests/copperline/run.sh

# Measure PBKDF2 throughput on a stock 68000 (informs the KDF policy). Dev-only:
# needs a Kickstart ROM (timer.device EClock isn't available under AROS).
pbkdf2-bench:
	sh tests/copperline/bench.sh

# --- guide: AmigaGuide user documentation, generated from the wiki -----------
# The GitHub wiki is the single source of truth for user docs; this converts
# it for on-Amiga reading (MultiView/AmigaGuide). Clone it as a sibling:
#   git clone https://github.com/sidick/amiauth.wiki.git ../amiauth.wiki
WIKI_DIR ?= ../amiauth.wiki
guide: | $(BUILD)
	@test -f $(WIKI_DIR)/Home.md || \
		{ echo "guide: wiki clone not found at $(WIKI_DIR) (set WIKI_DIR=)"; exit 1; }
	python3 tools/wiki2guide.py $(WIKI_DIR) $(BUILD)/AmiAuth.guide

# --- lha: build the real LHa for UNIX (archive-capable), pinned --------------
# Homebrew's and Ubuntu's `lha` is Lhasa — extract-only, useless for packaging
# — and the last lha *release* tag (2021) no longer compiles with modern
# compilers, so build a pinned master commit from source into build/tools/.
# Needs git + autoconf/automake. Override with a known-good archiver:
#   make dist LHA=/path/to/real/lha
LHA_REPO   := https://github.com/jca02266/lha.git
LHA_COMMIT := 86094cb56aba34de45668f39f74fcfb61e9d7fb6
LHA        ?= $(BUILD)/tools/lha

$(BUILD)/tools/lha:
	@mkdir -p $(BUILD)/tools
	rm -rf $(BUILD)/tools/lha-src
	git clone -q $(LHA_REPO) $(BUILD)/tools/lha-src
	cd $(BUILD)/tools/lha-src && \
		git -c advice.detachedHead=false checkout -q $(LHA_COMMIT) && \
		autoreconf -fi >/dev/null 2>&1 && ./configure >/dev/null && \
		$(MAKE) >/dev/null
	cp $(BUILD)/tools/lha-src/src/lha $(BUILD)/tools/lha
	rm -rf $(BUILD)/tools/lha-src

# --- dist: assemble the Aminet upload pair (archive + .readme) ---------------
# Expects prebuilt m68k binaries (make m68k-docker gui-docker); the lha
# archiver is built automatically (above). Produces build/dist/AmiAuth.lha
# (drawer with binaries, docs, icons) and build/dist/AmiAuth.readme alongside
# — the two files Aminet wants. Icons: the drawer icon sits next to the
# drawer; the CLI deliberately has no icon (it is a Shell command).
dist: guide $(LHA)
	@test -f $(BUILD)/AmiAuth -a -f $(BUILD)/AmiAuthGUI || \
		{ echo "dist: missing m68k binaries; run: make m68k-docker gui-docker"; exit 1; }
	rm -rf $(BUILD)/dist
	mkdir -p $(BUILD)/dist/AmiAuth
	cp $(BUILD)/AmiAuth $(BUILD)/AmiAuthGUI $(BUILD)/AmiAuth.guide \
		LICENSE THIRDPARTY.md AmiAuth.readme $(BUILD)/dist/AmiAuth/
	cp icons/AmiAuthGUI.info icons/AmiAuth.guide.info $(BUILD)/dist/AmiAuth/
	cp icons/AmiAuth.info AmiAuth.readme $(BUILD)/dist/
	cd $(BUILD)/dist && $(abspath $(LHA)) aq AmiAuth.lha AmiAuth AmiAuth.info
	@ls -l $(BUILD)/dist/AmiAuth.lha $(BUILD)/dist/AmiAuth.readme

# --- movepointer: dev/test-only tool, cross-built from vendored source -------
# tests/tools/movepointer/ (1987, Public Domain - see its README) precisely
# positions the mouse under Copperline, working around --mouse-after's
# non-linear host-to-guest scaling (see the copperline-testing skill). Built
# from source with our own toolchain rather than trusting the original
# Aminet prebuilt binary; -w since it's third-party (mirrors quirc's
# treatment) - the warnings are 1987 K&R implicit-declarations, harmless.
movepointer: | $(BUILD)
	$(M68K_CC) -w -m68000 -noixemul tests/tools/movepointer/movepointer.c \
		-lamiga -o $(BUILD)/MovePointer

movepointer-docker:
	$(DOCKER) run --rm --platform linux/amd64 -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make movepointer'

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
