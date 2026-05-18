# `rosbot` MAVLink dialect

This directory is the **single source of truth** for the MAVLink dialect used by
both the firmware and the bridge node. The generated C headers in
`generated/` are checked in for hermetic builds — flashing the firmware does
not require running `mavgen` on the build machine.

## Files

- `rosbot.xml` — dialect definition. Includes `common.xml`; custom messages
  start at ID 11000 (see [MAVLINK_MIGRATION.md](../../../MAVLINK_MIGRATION.md)
  §7).
- `common.xml`, `standard.xml`, `minimal.xml` — vendored copies from
  `pymavlink` (v2.0). They are the parents `rosbot.xml` pulls in transitively;
  vendoring them removes the `pymavlink` runtime dependency from contributors
  who only want to flash.
- `generated/` — checked-in mavgen output. Both the firmware (via the
  `-I lib/mavlink/dialect/generated/<flavour>` include path injected by
  `platformio.ini`) and the bridge package consume these headers.

## Regenerate after editing `rosbot.xml`

Pre-requisite: `pymavlink` >= 2.4 in a virtualenv. The repo's `justfile` has
a `install-deps` recipe that creates `~/.venv-mavlink` for you:

```bash
just install-deps        # one-time: pymavlink + platformio in ~/.venv-mavlink
just mavgen              # regenerate headers in place
```

Equivalent without `just`:

```bash
~/.venv-mavlink/bin/mavgen.py \
  --lang=C --wire-protocol=2.0 \
  --output=lib/mavlink/dialect/generated \
  lib/mavlink/dialect/rosbot.xml
```

Commit the regenerated `generated/` tree alongside the `rosbot.xml` change in
the same commit so PR reviewers see the wire-protocol diff.
