FS_UAE ?= /Applications/FS-UAE.app/Contents/MacOS/fs-uae
FS_UAE_CONFIG ?= $(CURDIR)/emulator/HappyAmigaClock.fs-uae
BOOT_ADF ?= $(CURDIR)/emulator/HappyAmigaClock-dev.adf
VBCC_ROOT ?= /Users/mark/Developer/Amiga/vbcc
NDK_INC ?= /Users/mark/Developer/Amiga/sdk/NDK_3.9/Include/include_h

# VBCC usa questa variabile per trovare config/ e targets/
export VBCC := $(VBCC_ROOT)

# Permette al Makefile.vbcc di trovare vc, vlink e vasmm68k_mot
export PATH := $(VBCC_ROOT)/bin:$(PATH)

# Se Makefile.vbcc usa questa variabile per gli header NDK
export NDK_INC

.PHONY: all build clean run check-vbcc

all: build

check-vbcc:
	@test -x "$(VBCC_ROOT)/bin/vc" || \
		(echo "Errore: vc non trovato in $(VBCC_ROOT)/bin"; exit 1)
	@test -x "$(VBCC_ROOT)/bin/vlink" || \
		(echo "Errore: vlink non trovato in $(VBCC_ROOT)/bin"; exit 1)

build: check-vbcc
	$(MAKE) -f Makefile.vbcc

clean:
	$(MAKE) -f Makefile.vbcc clean

run: build
	"$(FS_UAE)" "$(FS_UAE_CONFIG)" \
		--floppy-drive-0="$(BOOT_ADF)" \
		--hard-drive-1="$(CURDIR)/dist" \
		--hard-drive-1-label=HAC