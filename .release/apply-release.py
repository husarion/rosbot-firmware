#!/usr/bin/env python3
# Copyright 2026 Husarion sp. z o.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Apply a release: prepend a CHANGELOG section + bump FW_VERSION.

Usage:
  apply-release.py <full_tag> <section_file>

  full_tag      e.g. v0.1.0-jazzy-mavlink (must start with 'v').
  section_file  path to a file containing the Keep-a-Changelog body for
                this release (### Added / Changed / Fixed / Removed
                blocks). No top-level header — this script prepends it.

Writes:
  CHANGELOG.md   — new "## [<tag-without-v>] - YYYY-MM-DD" section at top.
  platformio.ini — FW_VERSION build flag replaced with the new tag.

Run from the repo root. Atomic-ish: writes happen one file at a time;
if the second one fails, re-running with --revert restores from the .bak
files. Drop the .bak files when you're confident.
"""

import datetime as dt
import re
import shutil
import sys
from pathlib import Path

CHANGELOG = Path("CHANGELOG.md")
PIO_INI = Path("platformio.ini")


def prepend_section(tag: str, section_body: str) -> None:
    if not CHANGELOG.is_file():
        sys.exit(f"{CHANGELOG} missing — bootstrap it before releasing.")
    version_key = tag.lstrip("v")
    today = dt.date.today().isoformat()
    new_section = f"## [{version_key}] - {today}\n\n{section_body.rstrip()}\n\n"

    text = CHANGELOG.read_text()
    m = re.search(r"(?m)^## \[", text)
    if m:
        new_text = text[: m.start()] + new_section + text[m.start() :]
    else:
        if not text.endswith("\n\n"):
            text = text.rstrip("\n") + "\n\n"
        new_text = text + new_section

    shutil.copy(CHANGELOG, CHANGELOG.with_suffix(CHANGELOG.suffix + ".bak"))
    CHANGELOG.write_text(new_text)
    print(f"prepended CHANGELOG.md section: ## [{version_key}] - {today}")


def bump_fw_version(tag: str) -> None:
    text = PIO_INI.read_text()
    pattern = re.compile(r'(-D FW_VERSION=\\")[^"\\]*(\\")')
    new_text, n = pattern.subn(rf"\g<1>{tag}\g<2>", text)
    if n == 0:
        sys.exit("FW_VERSION line not found in platformio.ini")
    shutil.copy(PIO_INI, PIO_INI.with_suffix(PIO_INI.suffix + ".bak"))
    PIO_INI.write_text(new_text)
    print(f"bumped FW_VERSION to {tag} in platformio.ini ({n} occurrence)")


def main() -> None:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    tag, section_file = sys.argv[1], Path(sys.argv[2])
    if not tag.startswith("v"):
        sys.exit(f"tag must start with 'v', got: {tag}")
    if not re.match(r"^v\d+\.\d+\.\d+(-[A-Za-z0-9.\-]+)?$", tag):
        sys.exit(f"tag must look like v<X>.<Y>.<Z>[-<suffix>], got: {tag}")
    if not section_file.is_file():
        sys.exit(f"section file not found: {section_file}")
    body = section_file.read_text()
    if not body.strip():
        sys.exit("section file is empty")

    prepend_section(tag, body)
    bump_fw_version(tag)


if __name__ == "__main__":
    main()
