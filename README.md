# Happy Amiga Clock

A Commodore Amiga application built with the m68k-amiga-elf GCC toolchain, using Intuition and Graphics libraries.

## Prerequisites

- [Bebbo's `m68k-amiga-elf` GCC toolchain](https://github.com/bebbo/amiga-gcc) — provides `m68k-amiga-elf-gcc`, `m68k-amiga-elf-as`, and `elf2hunk`
- [vasm](http://sun.hasenbraten.de/vasm/) (`vasmm68k_mot`) — assembles the `support/*.asm` sources
- GNU Make

All of the above are bundled if you use the [Amiga Support extension for VS Code](https://marketplace.visualstudio.com/items?itemName=BartmanAbyss.amiga-debug), which this project is configured for (see `.vscode/`).

## Building

### From VS Code

Open the folder in VS Code with the Amiga Support extension installed, then run the default build task (`Cmd/Ctrl+Shift+B`, or Terminal → Run Build Task). This runs `make` using the toolchain bundled with the extension.

### From the command line

Make sure `m68k-amiga-elf-gcc`, `m68k-amiga-elf-as`, `vasmm68k_mot`, and `elf2hunk` are on your `PATH`, then run:

```sh
make
```

This produces:

- `obj/` — intermediate object files
- `out/a.elf` — linked ELF binary
- `out/a.exe` — final Amiga executable (Hunk format, via `elf2hunk`)
- `out/a.s` — disassembly listing

To clean build artifacts:

```sh
make clean
```

## Running

Copy `out/a.exe` to a real Amiga, or run it under an emulator such as [FS-UAE](https://fs-uae.net/) or [WinUAE](https://www.winuae.net/).

## Project layout

- `main.c` — application entry point (opens an Intuition window)
- `support/` — GCC support routines and a data depacker (Doynax) in assembly
- `gfx/` — source images and conversion tools (`KingCon.exe`, `FreeImage.dll`) for generating `.bpl`/`.pal` graphics data
- `*.bpl` / `*.pal` — converted bitplane image and palette data
- `*.p61` — [The Player 6.1](http://www.abime.net/) module music, played via `player610.6.no_cia.bin`
