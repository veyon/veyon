#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
from collections import defaultdict
from pathlib import Path
from typing import Optional, List, Dict, Set


FRAMEWORK_FALLBACKS = {
    "QtHttpServer.framework": Path("/Users/pablo/GitHub/qt5httpserver-build/lib/QtHttpServer.framework"),
    "QtSslServer.framework": Path("/Users/pablo/GitHub/qt5httpserver-build/lib/QtSslServer.framework"),
}


def run_command(cmd: List[str], *, capture: bool = False) -> str:
    if capture:
        return subprocess.check_output(cmd, text=True)
    subprocess.run(cmd, check=True)
    return ""


def is_macho(file_path: Path) -> bool:
    try:
        output = run_command(["file", "-b", str(file_path)], capture=True)
    except subprocess.CalledProcessError:
        return False
    return "Mach-O" in output


def macho_files(contents_dir: Path) -> List[Path]:
    files: List[Path] = []
    for path in contents_dir.rglob("*"):
        if path.is_file() and is_macho(path):
            files.append(path)
    return files


def load_suffix_map(contents_dir: Path) -> Dict[str, Path]:
    suffix_map: Dict[str, Path] = {}
    for path in contents_dir.rglob("*"):
        if not path.is_file():
            continue
        _register_suffix_map_entry(path, contents_dir, suffix_map)
    return suffix_map


def _register_suffix_map_entry(path: Path, contents_dir: Path, suffix_map: Dict[str, Path]) -> None:
    parts = path.relative_to(contents_dir).parts
    for i in range(1, len(parts) + 1):
        key = "/".join(parts[-i:])
        suffix_map.setdefault(key, path)


def get_install_id(file_path: Path) -> Optional[str]:
    try:
        output = run_command(["otool", "-D", str(file_path)], capture=True)
    except subprocess.CalledProcessError:
        return None
    lines = [line.strip() for line in output.splitlines()]
    if len(lines) >= 2:
        return lines[1]
    return None


def parse_dependencies(file_path: Path) -> List[str]:
    output = run_command(["otool", "-L", str(file_path)], capture=True)
    lines = [line.strip() for line in output.splitlines()[1:]]
    deps: List[str] = []
    for line in lines:
        if not line:
            continue
        deps.append(line.split(" ")[0])
    return deps


def path_is_external(dep: str) -> bool:
    if dep.startswith("@"):
        return False
    if dep.startswith("/System/") or dep.startswith("/usr/lib/"):
        return False
    return dep.startswith("/")


def find_internal_target(dep: str, suffix_map: Dict[str, Path]) -> Optional[Path]:
    parts = Path(dep).parts
    for length in range(len(parts), 0, -1):
        key = "/".join(parts[-length:])
        if key in suffix_map:
            return suffix_map[key]
    return None


def ensure_local_copy(dep: str, contents_dir: Path, suffix_map: Dict[str, Path]) -> Optional[Path]:
    target = find_internal_target(dep, suffix_map)
    if target is not None:
        return target
    source = Path(dep)
    if not source.exists():
        framework_name: Optional[str] = None
        for part in Path(dep).parts:
            if part.endswith(".framework"):
                framework_name = part
                break
        if framework_name and framework_name in FRAMEWORK_FALLBACKS:
            fallback_path = FRAMEWORK_FALLBACKS[framework_name]
            if fallback_path.exists():
                dest_framework_dir = contents_dir / "Frameworks" / framework_name
                if not dest_framework_dir.exists():
                    shutil.copytree(fallback_path, dest_framework_dir)
                    for path in dest_framework_dir.rglob("*"):
                        if path.is_file():
                            _register_suffix_map_entry(path, contents_dir, suffix_map)
                relative_parts = Path(dep).parts
                framework_index = relative_parts.index(framework_name)
                if framework_index + 1 < len(relative_parts):
                    target_path = dest_framework_dir / Path(*relative_parts[framework_index + 1:])
                else:
                    target_path = dest_framework_dir
                if target_path.exists():
                    _register_suffix_map_entry(target_path, contents_dir, suffix_map)
                    return target_path
        return None
    dest_dir = contents_dir / "Frameworks"
    dest_dir.mkdir(parents=True, exist_ok=True)
    dest = dest_dir / source.name
    if not dest.exists():
        shutil.copy2(source, dest)
        os.chmod(dest, 0o755)
        _register_suffix_map_entry(dest, contents_dir, suffix_map)
    return dest


def relative_loader_path(from_path: Path, to_path: Path) -> str:
    rel = os.path.relpath(to_path, start=from_path.parent)
    return "@loader_path/" + rel.replace(os.sep, "/")


def needs_change(current: str) -> bool:
    return path_is_external(current)


def process_binary(file_path: Path, contents_dir: Path, suffix_map: Dict[str, Path]) -> bool:
    changed = False
    load_output = run_command(["otool", "-l", str(file_path)], capture=True)
    load_lines = [line.strip() for line in load_output.splitlines()]
    for index, line in enumerate(load_lines):
        if line == "cmd LC_RPATH" and index + 2 < len(load_lines):
            path_line = load_lines[index + 2]
            if path_line.startswith("path "):
                rpath = path_line.split("path", 1)[1].split("(", 1)[0].strip()
                if rpath.startswith("/usr/local") or rpath.startswith("/opt/local"):
                    run_command(["install_name_tool", "-delete_rpath", rpath, str(file_path)])
                    changed = True
    install_id = get_install_id(file_path)
    if install_id and needs_change(install_id):
        new_id = "@loader_path/" + file_path.name
        run_command(["install_name_tool", "-id", new_id, str(file_path)])
        changed = True
    deps = parse_dependencies(file_path)
    for dep in deps:
        if install_id and dep == install_id:
            continue
        if not path_is_external(dep):
            continue
        target = ensure_local_copy(dep, contents_dir, suffix_map)
        if target is None:
            continue
        new_path = relative_loader_path(file_path, target)
        run_command(["install_name_tool", "-change", dep, new_path, str(file_path)])
        changed = True
    return changed


def fix_bundle(bundle_dir: Path) -> Set[str]:
    contents_dir = bundle_dir / "Contents"
    if not contents_dir.is_dir():
        raise SystemExit(f"{bundle_dir} does not look like a .app bundle")
    suffix_map = load_suffix_map(contents_dir)
    remaining_external: Set[str] = set()
    progress = True
    while progress:
        progress = False
        for file_path in macho_files(contents_dir):
            if process_binary(file_path, contents_dir, suffix_map):
                progress = True
    # collect unresolved dependencies
    for file_path in macho_files(contents_dir):
        install_id = get_install_id(file_path)
        if install_id and needs_change(install_id):
            remaining_external.add(install_id)
        for dep in parse_dependencies(file_path):
            if install_id and dep == install_id:
                continue
            if path_is_external(dep):
                remaining_external.add(dep)
    return remaining_external


def main() -> None:
    parser = argparse.ArgumentParser(description="Fix bundled Mach-O dependencies to use @loader_path.")
    parser.add_argument("bundles", nargs="+", help="Path(s) to .app bundles.")
    args = parser.parse_args()
    unresolved: defaultdict[Path, Set[str]] = defaultdict(set)
    for bundle in args.bundles:
        bundle_path = Path(bundle).resolve()
        print(f"Processing {bundle_path} ...")
        missing = fix_bundle(bundle_path)
        if missing:
            unresolved[bundle_path].update(sorted(missing))
    if unresolved:
        print("Unresolved external references remain:")
        for bundle_path, deps in unresolved.items():
            for dep in sorted(deps):
                print(f"{bundle_path}: {dep}")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
