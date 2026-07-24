FS_UAE ?= /Applications/FS-UAE.app/Contents/MacOS/fs-uae
FS_UAE_CONFIG ?= $(CURDIR)/emulator/HappyAmigaClock.fs-uae
BOOT_ADF ?= $(CURDIR)/emulator/HappyAmigaClock-dev.adf
VBCC_ROOT ?= /Users/mark/Developer/Amiga/vbcc
NDK_INC ?= /Users/mark/Developer/Amiga/sdk/NDK_3.9/Include/include_h

export VBCC := $(VBCC_ROOT)
export PATH := $(VBCC_ROOT)/bin:$(PATH)
export NDK_INC

DIST_DIR ?= $(CURDIR)/dist
ICON_SOURCE := assets/HappyAmigaClock.info
ICON_TARGET := $(DIST_DIR)/HappyAmigaClock.info

.PHONY: all build clean run check-vbcc install-icon

all: build

check-vbcc:
	@test -x "$(VBCC_ROOT)/bin/vc" || \
		(echo "Errore: vc non trovato in $(VBCC_ROOT)/bin"; exit 1)
	@test -x "$(VBCC_ROOT)/bin/vlink" || \
		(echo "Errore: vlink non trovato in $(VBCC_ROOT)/bin"; exit 1)
	@test -x "$(VBCC_ROOT)/bin/vasmm68k_mot" || \
		(echo "Errore: vasmm68k_mot non trovato in $(VBCC_ROOT)/bin"; exit 1)

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
	rm -f "$(ICON_TARGET)"

run: build
	"$(FS_UAE)" "$(FS_UAE_CONFIG)" \
		--floppy-drive-0="$(BOOT_ADF)" \
		--hard-drive-1="$(CURDIR)/dist" \
		--hard-drive-1-label=HAC