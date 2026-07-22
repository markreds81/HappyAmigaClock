FS_UAE ?= /Applications/FS-UAE.app/Contents/MacOS/fs-uae
FS_UAE_CONFIG ?= $(CURDIR)/emulator/HappyAmigaClock.fs-uae
BOOT_ADF ?= $(CURDIR)/emulator/HappyAmigaClock-dev.adf

.PHONY: all build clean shell run

all: build

build:
	docker compose run --rm vbcc make -f Makefile.vbcc

clean:
	docker compose run --rm vbcc make -f Makefile.vbcc clean

shell:
	docker compose run --rm vbcc /bin/bash

run: build
	"$(FS_UAE)" "$(FS_UAE_CONFIG)" \
		--floppy-drive-0="$(BOOT_ADF)" \
		--hard-drive-1="$(CURDIR)/dist" \
		--hard-drive-1-label=HAC