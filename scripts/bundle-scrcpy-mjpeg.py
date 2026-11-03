#!/usr/bin/env python3
"""Copy a scrcpy macOS client with its Homebrew dylibs into one directory."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def run(*args: str) -> str:
    return subprocess.check_output(args, text=True)


def dylibs(path: Path) -> list[str]:
    lines = run("otool", "-L", str(path)).splitlines()[1:]
    return [line.strip().split(" (", 1)[0] for line in lines if line.strip()]


def is_homebrew(name: str) -> bool:
    return name.startswith("/opt/homebrew/")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("client", type=Path, help="built scrcpy executable")
    parser.add_argument("destination", type=Path, help="empty output directory")
    args = parser.parse_args()
    client = args.client.resolve(strict=True)
    dest = args.destination.resolve()
    if dest.exists() and any(dest.iterdir()):
        parser.error(f"destination is not empty: {dest}")
    libdir = dest / "lib"
    libdir.mkdir(parents=True, exist_ok=True)
    target = dest / "scrcpy"
    shutil.copy2(client, target)
    os.chmod(target, 0o755)

    pending = [client]
    sources: dict[str, Path] = {}
    while pending:
        current = pending.pop()
        for name in dylibs(current):
            if not is_homebrew(name):
                continue
            source = Path(name).resolve(strict=True)
            if source == current.resolve():  # dylib install ID
                continue
            basename = Path(name).name
            prior = sources.get(basename)
            if prior and prior != source:
                raise RuntimeError(f"library name collision: {prior} vs {source}")
            if not prior:
                sources[basename] = source
                pending.append(source)

    for basename, source in sorted(sources.items()):
        copied = libdir / basename
        shutil.copy2(source, copied)
        os.chmod(copied, 0o755)

    for binary in [target, *sorted(libdir.iterdir())]:
        loader = "@loader_path/lib" if binary == target else "@loader_path"
        changes: list[str] = []
        for name in dylibs(binary):
            if is_homebrew(name):
                changes += ["-change", name, f"{loader}/{Path(name).name}"]
        if binary != target:
            changes += ["-id", f"@rpath/{binary.name}"]
        if changes:
            subprocess.run(["install_name_tool", *changes, str(binary)], check=True)
        subprocess.run(
            ["codesign", "--force", "--sign", "-", "--timestamp=none", str(binary)],
            check=True,
            stdout=subprocess.DEVNULL,
        )

    print(f"Bundled {len(sources)} Homebrew dylibs in {dest}")


if __name__ == "__main__":
    main()
