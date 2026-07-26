#!/usr/bin/env python3
"""Create a level-0 LHA archive containing uncompressed files."""

import argparse
import datetime
import struct
from pathlib import Path


def crc16(data: bytes) -> int:
    crc = 0
    for value in data:
        crc ^= value
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def dos_timestamp(path: Path) -> int:
    stamp = datetime.datetime.fromtimestamp(path.stat().st_mtime)
    year = min(max(stamp.year, 1980), 2107)
    return (
        ((year - 1980) << 25)
        | (stamp.month << 21)
        | (stamp.day << 16)
        | (stamp.hour << 11)
        | (stamp.minute << 5)
        | (stamp.second // 2)
    )


def archive_entry(path: Path) -> bytes:
    data = path.read_bytes()
    name = path.name.encode("ascii")

    if len(name) > 255:
        raise ValueError(f"filename too long for LHA level 0: {path.name}")

    header = (
        b"-lh0-"
        + struct.pack("<II", len(data), len(data))
        + struct.pack("<I", dos_timestamp(path))
        + bytes((0x20, 0, len(name)))
        + name
        + struct.pack("<H", crc16(data))
    )

    if len(header) > 255:
        raise ValueError(f"header too long for LHA level 0: {path.name}")

    return bytes((len(header), sum(header) & 0xFF)) + header + data


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Create an uncompressed level-0 LHA archive."
    )
    parser.add_argument("output", type=Path)
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()

    missing = [str(path) for path in args.files if not path.is_file()]
    if missing:
        parser.error("file not found: " + ", ".join(missing))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("wb") as output:
        for path in args.files:
            output.write(archive_entry(path))
        output.write(b"\0")

    print(f"Created {args.output}")


if __name__ == "__main__":
    main()
