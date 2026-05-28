# `rosbot` MAVLink dialect

This directory holds the dialect **definition** (the XML inputs to mavgen).
The **generated C headers** live inside the bridge package at
[`bridge/rosbot_mavlink_bridge/mavlink_dialect/`](../../../bridge/rosbot_mavlink_bridge/mavlink_dialect/)
— single canonical location. Both consumers reach them from there:

- **Firmware** — via the `-I bridge/rosbot_mavlink_bridge/mavlink_dialect/rosbot`
  flag in [`platformio.ini`](../../../platformio.ini).
- **Bridge node** — via its own `CMakeLists.txt` (the headers ship inside
  the package, which keeps the bridge self-contained for bloom releases).

The generated headers are checked in for hermetic builds — flashing the
firmware does not require running `mavgen` on the build machine.

## Files in this directory

- `rosbot.xml` — dialect definition. Includes `common.xml`; custom messages
  start at ID 11000 (see [MAVLINK_MIGRATION.md](../../../MAVLINK_MIGRATION.md)
  §7).
- `common.xml`, `standard.xml`, `minimal.xml` — vendored copies from
  `pymavlink` (v2.0). They are the parents `rosbot.xml` pulls in transitively;
  vendoring them removes the `pymavlink` runtime dependency from contributors
  who only want to flash.

## Regenerate after editing `rosbot.xml`

Pre-requisite: `pymavlink` >= 2.4 in a virtualenv. The repo's `justfile` has
an `install-deps` recipe that creates `~/.venv-mavlink` for you:

```bash
just install-deps        # one-time: pymavlink + platformio in ~/.venv-mavlink
just mavgen              # regenerate headers into bridge/.../mavlink_dialect/
```

Equivalent without `just`:

```bash
rm -rf bridge/rosbot_mavlink_bridge/mavlink_dialect
~/.venv-mavlink/bin/mavgen.py \
  --lang=C --wire-protocol=2.0 \
  --output=bridge/rosbot_mavlink_bridge/mavlink_dialect \
  lib/mavlink/dialect/rosbot.xml
```

Commit the regenerated `mavlink_dialect/` tree alongside the `rosbot.xml`
change in the same commit so PR reviewers see the wire-protocol diff.
CI's `verify-dialect` job re-runs `mavgen` and diffs against the committed
output to catch a missed regen.
