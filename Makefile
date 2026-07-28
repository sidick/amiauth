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

# GUI window title shows the commit hash unless this build is exactly on a
# release tag (the tag-driven release workflow always builds from a `vX.Y`
# tag, so "on a tag at all" is a reliable proxy for "this is the release
# build" in this repo). Computed on the host so gui-docker doesn't need git
# inside the container - passed through as plain make variables instead.
GIT_HASH   ?= $(shell git rev-parse --short HEAD 2>/dev/null)
GIT_ON_TAG ?= $(shell git describe --tags --exact-match >/dev/null 2>&1 && echo 1)
ifneq ($(GIT_ON_TAG),1)
VERSION_DEFS := -DAMIAUTH_BUILD_HASH=\"$(GIT_HASH)\"
endif

# Containerised cross-build: same image as CI, so local m68k builds match.
DOCKER          ?= docker
AMIGA_GCC_IMAGE ?= ghcr.io/reinauer/container-amiga-gcc:latest
# Run as the calling user, not root: the container bind-mounts $(CURDIR), and
# without this, files it creates (build/, the m68k binaries) come out root-
# owned on Linux hosts - breaking any later non-Docker step (e.g. `make dist`)
# that needs to write into the same build/ directory. Docker Desktop on macOS
# has more forgiving bind-mount semantics, so this doesn't reproduce locally
# on every platform - found via a real CI release-workflow rehearsal (#38).
DOCKER_USER     := --user "$(shell id -u):$(shell id -g)"

CORE_SRCS  := $(wildcard src/core/*.c)
TEST_SRCS  := $(wildcard tests/*.c)
CLI_SRCS   := src/cli/main.c

# Hand-written m68k asm for the crypto hot loops (#47) - m68k builds only,
# never the host (CORE_SRCS' *.c wildcard doesn't pick these up, so nothing
# extra is needed to keep them out of `make test`/`make cli`).
ASM_SRCS   := $(wildcard src/core/*.s)

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

# --- Vendored QR encoder (qrcodegen, MIT) + our portable wrapper ------------
# qrcodegen is third-party (warnings off, same reasoning as quirc above); it's
# integer-only so no float flags are needed. Reuses the qr-host/qr-m68k object
# pattern rules below (they match any src/qr/%.c, qrcodegen.c included).
QRENC_WRAP      := src/qr/qrencode.c
DIFF_SRCS  := tests/diff/diff_main.c
# AmigaOS-only front-end glue (bsdsocket SNTP, ...); m68k build only. qrimage.c
# and arexx.c are GUI-only (datatypes.library / the resident ARexx port -
# the CLI is a one-shot process, nothing to serve) so both are excluded here
# and added to GUI_SRCS instead.
AMIGA_SRCS := $(filter-out src/amiga/qrimage.c src/amiga/arexx.c,$(wildcard src/amiga/*.c))

# OpenSSL flags for the differential harness (pkg-config, with a plain fallback).
OPENSSL_CFLAGS ?= $(shell pkg-config --cflags libcrypto 2>/dev/null)
OPENSSL_LIBS   ?= $(shell pkg-config --libs libcrypto 2>/dev/null || echo -lcrypto)

# Iterations/primitive for `make diff` (override: make diff DIFF_ITERS=20000).
DIFF_ITERS ?= 5000

BUILD := build

# Object paths for the vendored quirc decoder (needs $(BUILD), defined above).
QUIRC_HOST_OBJS := $(patsubst src/qr/%.c,$(BUILD)/qr-host/%.o,$(QUIRC_SRCS))
QUIRC_M68K_OBJS := $(patsubst src/qr/%.c,$(BUILD)/qr-m68k/%.o,$(QUIRC_SRCS))

# Object paths for the vendored qrcodegen encoder (same qr-host/qr-m68k rules).
QRCODEGEN_HOST_OBJ := $(BUILD)/qr-host/qrcodegen.o
QRCODEGEN_M68K_OBJ := $(BUILD)/qr-m68k/qrcodegen.o

.PHONY: all test cli smoke diff m68k m68k-docker gui gui-docker gui-smoke qr-onhw qr-onhw-docker qr-onhw-smoke arexx-onhw arexx-onhw-docker arexx-onhw-smoke serialtest-m68k serialtest-m68k-docker copperline-smoke pbkdf2-bench asm-bench amissl-bench flexcat flexcat-docker catalog-strings catalog-strings-docker check-catalog catalog-onhw-smoke catalog-nolib-onhw catalog-nolib-onhw-docker clean

all: test cli

# --- Host: unit / RFC-vector tests ---
test: $(BUILD)/run-tests
	VAULT_TEST_FILE=$(BUILD)/amiauth-test.vault \
		AMIAUTH_PREFS_DIR=$(BUILD)/prefs-test $(BUILD)/run-tests

$(BUILD)/run-tests: $(CORE_SRCS) $(TEST_SRCS) $(QR_WRAP) $(QUIRC_HOST_OBJS) $(QRENC_WRAP) $(QRCODEGEN_HOST_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) $(QR_CPPFLAGS) -Itests \
		$(CORE_SRCS) $(TEST_SRCS) $(QR_WRAP) $(QUIRC_HOST_OBJS) \
		$(QRENC_WRAP) $(QRCODEGEN_HOST_OBJ) -o $@

# Vendored quirc objects — host toolchain, warnings suppressed (third-party).
$(BUILD)/qr-host/%.o: src/qr/%.c | $(BUILD)
	@mkdir -p $(BUILD)/qr-host
	$(CC) -std=c99 -O2 -w $(QR_CPPFLAGS) -c $< -o $@

# --- Host: native CLI (for local development) ---
# Named distinctly from the m68k 'AmiAuth' binary so the two don't collide on a
# case-insensitive filesystem (macOS). Includes the QR encoder (#45's CLI QR
# command): our qrencode.c wrapper + the vendored qrcodegen object.
cli: $(BUILD)/amiauth-host

$(BUILD)/amiauth-host: $(CORE_SRCS) $(CLI_SRCS) $(QRENC_WRAP) $(QRCODEGEN_HOST_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $(CObjINC) -Isrc/qr $(CORE_SRCS) $(CLI_SRCS) \
		$(QRENC_WRAP) $(QRCODEGEN_HOST_OBJ) -o $@

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
# Includes the QR encoder (#45's CLI QR command): see the qr-m68k object rule
# below (shared with the GUI's decoder build).
m68k: $(QRCODEGEN_M68K_OBJ) | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) -Isrc/qr $(CORE_SRCS) $(ASM_SRCS) $(AMIGA_SRCS) $(CLI_SRCS) \
		$(QRENC_WRAP) $(QRCODEGEN_M68K_OBJ) -o $(BUILD)/AmiAuth

# Test-only CLI variant (#67): built with a bogus locale.library name, so
# catalog_open()'s OpenLibrary() genuinely fails and the on-target catalog
# test can exercise the LocaleBase==NULL fallback path without deleting the
# real locale.library from a WB clone (which breaks unrelated boot
# components - see tests/gui/catalog-onhw.sh). Never shipped.
CATALOG_NOLIB_DEFS := -DAMIAUTH_LOCALE_LIBNAME='"locale.library.nonexistent"'

catalog-nolib-onhw: $(QRCODEGEN_M68K_OBJ) | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(CATALOG_NOLIB_DEFS) -Isrc/qr $(CORE_SRCS) $(ASM_SRCS) $(AMIGA_SRCS) $(CLI_SRCS) \
		$(QRENC_WRAP) $(QRCODEGEN_M68K_OBJ) -o $(BUILD)/AmiAuth-nolib

catalog-nolib-onhw-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make catalog-nolib-onhw'

# --- m68k: ReAction GUI binary (Amiga only; needs intuition + ReAction classes) ---
# Includes the QR decoder: qrimage.c (datatypes glue) + our qr.c wrapper + the
# vendored quirc objects (built -w for m68k). QUIRC_FLOAT_TYPE=float: no FPU.
# arexx.c (#46) is the ARexx port's RexxMsg glue - GUI-only, see AMIGA_SRCS.
GUI_SRCS := src/gui/main.c src/amiga/qrimage.c src/amiga/arexx.c

# Vendored quirc objects — m68k toolchain, warnings suppressed (third-party).
$(BUILD)/qr-m68k/%.o: src/qr/%.c | $(BUILD)
	@mkdir -p $(BUILD)/qr-m68k
	$(M68K_CC) -std=c99 -O2 -m68000 -noixemul -w $(QR_CPPFLAGS) -c $< -o $@

gui: $(QUIRC_M68K_OBJS) $(QRCODEGEN_M68K_OBJ) | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(VERSION_DEFS) $(QR_CPPFLAGS) $(CORE_SRCS) $(ASM_SRCS) $(AMIGA_SRCS) $(GUI_SRCS) \
		$(QR_WRAP) $(QUIRC_M68K_OBJS) $(QRENC_WRAP) $(QRCODEGEN_M68K_OBJ) -lm -lamiga -o $(BUILD)/AmiAuthGUI

gui-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make gui GIT_HASH=$(GIT_HASH) GIT_ON_TAG=$(GIT_ON_TAG)'

# --- Localization (#67): FlexCat (github.com/adtools/flexcat) generates
# src/core/catalog_strings.h from locale/AmiAuth.cd. FlexCat itself is a
# host-only build tool (never ships, never touches the m68k cross-compiler -
# see tools/fetch-flexcat.sh) and the generated header is checked into git
# like any other source file, so ordinary `make test`/`cli`/`m68k`/`gui`
# builds need neither FlexCat nor network access. Only re-run `make
# catalog-strings` (regenerating + committing the header) after editing
# locale/AmiAuth.cd. Needs a native (non-cross) C compiler on PATH - macOS's
# clang chokes on FlexCat's own build flags, so use `flexcat-docker` there.
flexcat: $(BUILD)/flexcat

$(BUILD)/flexcat: | $(BUILD)
	src=$$(bash tools/fetch-flexcat.sh) && \
	cp "$$src" $(BUILD)/flexcat && chmod +x $(BUILD)/flexcat && \
	cp "$$(dirname $$src)/../sd/CatComp_h.sd" $(BUILD)/CatComp_h.sd

flexcat-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'make flexcat'

catalog-strings: $(BUILD)/flexcat
	$(BUILD)/flexcat locale/AmiAuth.cd src/core/catalog_strings.h=$(BUILD)/CatComp_h.sd

# $(BUILD)/flexcat is a Linux ELF binary (built inside the container) - won't
# run directly on a macOS host, so regenerating the header locally on macOS
# needs the whole step done inside the container, not just the build half.
catalog-strings-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'make catalog-strings'

# Structural sanity checks on locale/AmiAuth.cd and any .ct translation -
# placeholder consistency, button-mnemonic uniqueness, CLI re-key prompt
# markers (tools/check_catalog.py). Pure Python, no FlexCat/Docker needed.
# Runs against every .ct this repo has, including unreviewed locale/drafts/
# ones - these are purely structural checks, not a translation-quality
# review, so there's no reason to exempt a draft from them.
check-catalog:
	python3 tools/check_catalog.py locale/AmiAuth.cd $(wildcard locale/*.ct) $(wildcard locale/drafts/*.ct)

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
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make qr-onhw QR_ONHW_DEFS=$(QR_ONHW_DEFS)'

qr-onhw-smoke:
	sh tests/gui/qr-onhw.sh

# --- m68k via the CI container: no local toolchain needed, matches CI exactly ---
m68k-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make m68k'

# --- m68k asm crypto tests (#47): the hand-written SHA-1 hot loop, validated
# against every existing SHA-1/HMAC/PBKDF2 RFC vector by forcing the dispatch
# pointer onto the asm before running them (tests/asm/). ChaCha20 has no asm
# path - see src/core/crypto_dispatch.h. Run under amitools' vamos (a
# separate, optional install: pip install 'amitools[vamos]' - not needed for
# any other target here).
asm-tests: $(BUILD)/asm-test-sha1

$(BUILD)/asm-test-sha1: | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) -Itests tests/asm/test_sha1_asm.c \
		tests/test_sha1.c tests/test_hmac.c tests/test_pbkdf2.c tests/test_kdf.c \
		$(CORE_SRCS) $(ASM_SRCS) src/amiga/prefs.c -lamiga -o $@

asm-tests-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make asm-tests'

# --- Copperline: headless on-target core smoke test (spike) ------------------
# Boots a stock A500/68000 under Copperline, runs the RFC 4226 HOTP vectors on
# real m68k, and checks the codes it emits over serial. See tests/copperline.
# OTP core chain (hotp_sha1 -> hmac -> sha1) + the DRBG; no vault/prefs/front-end.
SERIALTEST_SRCS := src/core/otp.c src/core/hmac.c src/core/sha1.c \
                   src/core/sha256.c src/core/sha512.c src/core/steamguard.c \
                   src/core/drbg.c tests/copperline/serialtest.c

serialtest-m68k: | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) $(SERIALTEST_SRCS) -o $(BUILD)/serialtest

serialtest-m68k-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make serialtest-m68k'

copperline-smoke: serialtest-m68k-docker
	sh tests/copperline/run.sh

# --- Headless on-target ARexx port test: boot WB 3.2, launch AmiAuthGUI
# resident, run a real ARexx script (tests/copperline/arexx-probe.rexx) via
# the WB image's resident RexxMast (`rx`) against AMIAUTH.1, then relay its
# redirected output back over serial with this small m68k program (arexxtest).
# See tests/gui/arexx-onhw.sh.
arexx-onhw: | $(BUILD)
	$(M68K_CC) $(M68K_CFLAGS) tests/copperline/arexxtest.c -o $(BUILD)/arexxtest

arexx-onhw-docker:
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make arexx-onhw'

arexx-onhw-smoke:
	sh tests/gui/arexx-onhw.sh

# --- Headless on-target catalog test (#67): boot WB 3.2, run AmiAuth CLI
# commands whose output is routed through MSG()/catalog_get(), with
# locale.library both present (no matching catalog - the common case) and
# entirely absent, asserting the correct English default either way. See
# tests/gui/catalog-onhw.sh for what this does/doesn't cover.
catalog-onhw-smoke:
	sh tests/gui/catalog-onhw.sh

# Measure PBKDF2 throughput on a stock 68000 (informs the KDF policy). Dev-only:
# needs a Kickstart ROM (timer.device EClock isn't available under AROS).
pbkdf2-bench:
	sh tests/copperline/bench.sh

# Compare the portable C SHA-1 compress function against the hand-written
# 68000 asm one (#47), same boot/CPU. Dev-only, verifies the asm path is
# actually worth its runtime-dispatch complexity.
asm-bench:
	sh tests/copperline/asm-bench.sh

# Compare AmiSSL's PBKDF2-HMAC-SHA1 against the builtin one, same boot/CPU
# (issue #85 groundwork). Dev-only: needs tests/gui/.env (AMIAUTH_WB_HDD/
# AMIAUTH_ROM) and the AmiSSL SDK (auto-fetched, cached).
amissl-bench:
	sh tests/copperline/amissl-bench.sh

# --- guide: AmigaGuide user documentation, generated from userdocs/ ----------
# userdocs/ is the single source of truth for user docs (published as the
# MkDocs site); this converts it for on-Amiga reading (MultiView/AmigaGuide).
guide: | $(BUILD)
	python3 tools/docs2guide.py userdocs $(BUILD)/AmiAuth.guide

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
	$(DOCKER) run --rm --platform linux/amd64 $(DOCKER_USER) -v "$(CURDIR)":/work -w /work \
		$(AMIGA_GCC_IMAGE) sh -lc 'PATH=/opt/amiga/bin:$$PATH make movepointer'

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
