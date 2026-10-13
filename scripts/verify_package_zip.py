#!/usr/bin/env python3
"""Checks an Onedow release ZIP against scripts/package_manifest.txt.

  python3 scripts/verify_package_zip.py bin/Onedow.zip [--require macOS/Onedow]
                                        [--no-exec-check] [--list]

Fails when the ZIP holds anything outside the manifest (saves, crash logs,
Screenshots, debug files...), misses a required item, or stores an `exec`
item without Unix execute permission.
"""
import argparse
import os
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOP = "Onedow/"


def read_manifest(path):
    items = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            parts = line.split()
            if not parts or parts[0].startswith("#"):
                continue
            kind, rel = parts[0], parts[1]
            if kind not in ("required", "optional"):
                raise SystemExit(f"manifest: unknown kind {kind!r}")
            items.append((kind, rel, "exec" in parts[2:]))
    return items


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("zip")
    ap.add_argument("--manifest", default=os.path.join(ROOT, "scripts", "package_manifest.txt"))
    ap.add_argument("--require", action="append", default=[],
                    help="manifest path that must be present in this ZIP")
    ap.add_argument("--no-exec-check", action="store_true",
                    help="skip the Unix permission check (ZIP made by a tool that cannot record it)")
    ap.add_argument("--list", action="store_true", help="print the file entries")
    args = ap.parse_args()

    items = read_manifest(args.manifest)
    errors = []
    with zipfile.ZipFile(args.zip) as z:
        infos = z.infolist()
    files = {}
    for info in infos:
        name = info.filename
        if not name.startswith(TOP):
            errors.append(f"outside {TOP}: {name}")
            continue
        if not name.endswith("/"):
            files[name[len(TOP):]] = info

    def allowed(rel):
        for _, item, _ in items:
            if item.endswith("/") and rel.startswith(item):
                return True
            if rel == item:
                return True
        return False

    def present(item):
        if item.endswith("/"):
            return any(rel.startswith(item) for rel in files)
        return item in files

    for rel in sorted(files):
        if not allowed(rel):
            errors.append(f"not in manifest: {rel}")
    for kind, item, exec_bit in items:
        if kind == "required" and not present(item):
            errors.append(f"missing required: {item}")
        if exec_bit and item in files and not args.no_exec_check:
            info = files[item]
            mode = info.external_attr >> 16
            if info.create_system != 3 or not mode & 0o111:
                errors.append(f"not executable: {item} (host={info.create_system}, mode={oct(mode)})")
    for item in args.require:
        if not present(item):
            errors.append(f"missing platform item: {item}")

    if args.list:
        for rel in sorted(files):
            print(rel)
    if errors:
        for e in errors:
            print("FAIL:", e, file=sys.stderr)
        return 1
    print(f"PASS: {args.zip} - {len(files)} files, all within the manifest")
    return 0


if __name__ == "__main__":
    sys.exit(main())
