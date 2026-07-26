FS_UAE ?= /Applications/FS-UAE.app/Contents/MacOS/fs-uae
FS_UAE_CONFIG ?= $(CURDIR)/emulator/HappyAmigaClock.fs-uae
BOOT_ADF ?= $(CURDIR)/emulator/HappyAmigaClock-dev.adf
VBCC_ROOT ?= /Users/mark/Developer/Amiga/vbcc
NDK_INC ?= /Users/mark/Developer/Amiga/sdk/NDK_3.9/Include/include_h
CLANG ?= clang
CLANG_FORMAT ?= clang-format

export VBCC := $(VBCC_ROOT)
export PATH := $(VBCC_ROOT)/bin:$(PATH)
export NDK_INC

DIST_DIR ?= $(CURDIR)/dist
ICON_SOURCE := assets/HappyAmigaClock.info
ICON_TARGET := $(DIST_DIR)/HappyAmigaClock.info
LHA_TARGET := $(DIST_DIR)/HappyAmigaClock.lha
LHA_TOOL := tools/create_lha.py
LINT_SOURCES := $(sort $(wildcard src/*.c))
FORMAT_SOURCES := $(sort $(wildcard include/*.h src/*.c))
AMIGA_REGISTERS := a0 a1 a2 a3 a4 a5 a6 d0 d1 d2 d3 d4 d5 d6 d7
AMIGA_REGISTER_DEFINES := $(foreach reg,$(AMIGA_REGISTERS),-D__$(reg)=)
LINTFLAGS := -fsyntax-only -std=c89 -Wall -Wextra -Wpedantic -Wshadow \
	-Werror -Wno-long-long -Wno-unknown-pragmas -Wno-pointer-sign \
	-Wno-incompatible-pointer-types-discards-qualifiers \
	-D__SASC -D__asm= $(AMIGA_REGISTER_DEFINES) \
	-Iinclude -isystem $(NDK_INC) -include intuition/intuitionbase.h

.PHONY: all build clean run check-vbcc check-lint check-format format \
	install-icon lha lint

all: build

check-vbcc:
	@test -x "$(VBCC_ROOT)/bin/vc" || \
		(echo "Errore: vc non trovato in $(VBCC_ROOT)/bin"; exit 1)
	@test -x "$(VBCC_ROOT)/bin/vlink" || \
		(echo "Errore: vlink non trovato in $(VBCC_ROOT)/bin"; exit 1)
	@test -x "$(VBCC_ROOT)/bin/vasmm68k_mot" || \
		(echo "Errore: vasmm68k_mot non trovato in $(VBCC_ROOT)/bin"; exit 1)

check-lint:
	@command -v "$(CLANG)" >/dev/null 2>&1 || \
		(echo "Errore: $(CLANG) non trovato"; exit 1)

lint: check-lint
	$(CLANG) $(LINTFLAGS) $(LINT_SOURCES)

check-format:
	@command -v "$(CLANG_FORMAT)" >/dev/null 2>&1 || \
		(echo "Errore: $(CLANG_FORMAT) non trovato"; exit 1)

format: check-format
	$(CLANG_FORMAT) -i $(FORMAT_SOURCES)

build: check-vbcc
	$(MAKE) -f Makefile.vbcc
	$(MAKE) install-icon

install-icon: $(ICON_TARGET)

$(ICON_TARGET): $(ICON_SOURCE) | $(DIST_DIR)
	cp "$<" "$@"

$(DIST_DIR):
	mkdir -p "$@"

clean:
	$(MAKE) -f Makefile.vbcc clean
	rm -f "$(ICON_TARGET)" "$(LHA_TARGET)"

lha: build
	python3 "$(LHA_TOOL)" "$(LHA_TARGET)" \
		"$(DIST_DIR)/HappyAmigaClock" \
		"$(ICON_TARGET)"

run: build
	"$(FS_UAE)" "$(FS_UAE_CONFIG)" \
		--floppy-drive-0="$(BOOT_ADF)" \
		--hard-drive-1="$(CURDIR)/dist" \
		--hard-drive-1-label=HAC
