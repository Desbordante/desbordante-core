import os
import re

ROOT = "src"

RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
RESET = "\033[0m"

# Include directories from old CMLs
INCLUDE_DIRS = [
    "core",
    "core/algorithms",
    "core/model",
    "core/model/types",
    "core/parser",
    "core/util",
    "core/config",
    "tests/common",
    "python_bindings"
]


def collect_header_paths():
    headers = set()
    for dirpath, _, filenames in os.walk(ROOT):
        for fn in filenames:
            if fn.endswith(".h"):
                rel = os.path.relpath(os.path.join(dirpath, fn), ROOT)
                headers.add(rel)
    return headers


def normalize_include_path(include, current_dir):
    combined = os.path.normpath(os.path.join(current_dir, include))
    return combined


def find_include_candidate(include, current_dir, headers):
    local_candidate = normalize_include_path(include, current_dir)
    has_local = local_candidate in headers

    core_candidates = []
    for core_dir in INCLUDE_DIRS:
        candidate = f"{core_dir}/{include}"
        if candidate in headers:
            core_candidates.append(candidate)

    candidates = []
    if has_local:
        candidates.append(local_candidate)
    candidates.extend(core_candidates)

    if has_local:
        if (local_candidate in core_candidates) or (not core_candidates):
            return local_candidate, False, None
        else:
            return None, True, candidates
    if core_candidates:
        if len(core_candidates) == 1:
            return core_candidates[0], False, None
        else:
            return None, True, candidates
    return None, False, None


def replace_includes_in_file(filepath, replacements):
    # `newline=''` to process CRLF files
    with open(filepath, "r", encoding="utf-8", newline='') as f:
        text = f.read()

    for old, new in replacements.items():
        text = text.replace(f'"{old}"', f'"{new}"')

    with open(filepath, "w", encoding="utf-8", newline='') as f:
        f.write(text)


def process_file(headers):
    include_re = re.compile(r'#\s*include\s*"([^"]+)"')

    for dirpath, _, filenames in os.walk(ROOT):
        for fn in filenames:
            if not fn.endswith((".cpp", ".h")):
                continue

            full_path = os.path.join(dirpath, fn)
            rel_file = os.path.relpath(full_path, ROOT)
            current_dir_rel = os.path.dirname(rel_file)

            with open(full_path, "r", encoding="utf-8", newline='') as f:
                lines = f.readlines()

            replacements = {}
            for lineno, line in enumerate(lines, start=1):
                match = include_re.search(line)
                if not match:
                    continue

                inc_raw = match.group(1)

                if inc_raw.startswith(("core/", "python_bindings/", "tests/")):
                    continue

                chosen, is_conflict, candidates = find_include_candidate(inc_raw, current_dir_rel, headers)

                if is_conflict:
                    print(f"\n{CYAN}File:{RESET} {rel_file}:{lineno}\n"
                          f"  {RED}Conflict:{RESET} {YELLOW}{inc_raw}{RESET} matches multiple targets.\n"
                          f"  Candidates:\n    - " + "\n    - ".join(candidates)
                          )
                elif chosen:
                    print(f"\n{CYAN}File:{RESET} {rel_file}:{lineno}\n"
                          f"  {YELLOW}{inc_raw}{RESET} → {GREEN}{chosen}{RESET}"
                          )
                    replacements[inc_raw] = chosen
                else:
                    print(f"\n{CYAN}File:{RESET} {rel_file}:{lineno}\n"
                          f"  {RED}Not found:{RESET} {YELLOW}{inc_raw}{RESET}"
                          )

            if replacements:
                replace_includes_in_file(full_path, replacements)


if __name__ == "__main__":
    all_headers = collect_header_paths()
    process_file(all_headers)
