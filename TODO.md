# TODO

Open work items not yet tracked as GitHub issues. Newest at the top.
Move items out (to a release or to `git rm` after merging) once they ship.

---

## MAVLink boot banner never replays after fresh boot — bridge always WARNs

**Symptom.** After `rosbot.flash` (or any cold MCU boot followed by a
delayed daemon start), `rosbot_mavlink_bridge` logs:

```
Still waiting for firmware boot banner matching 'rosbot_xl .* mavlink'.
No boot banner seen within 8s of first HEARTBEAT — promoting anyway
(firmware likely booted before bridge).
```

Functionally fine (the bridge's graceful fallback promotes anyway and
driving/TIMESYNC/IMU/odom all work), but the WARN fires on every
startup and we lose the firmware-string verification it was designed
to provide.

**Root cause — state-machine latch in `lib/mavlink/mavlink_node.cpp`.**
The MCU emits the boot banner up to `kBannerAttempts = 10` times at 1 s
intervals after its own first HEARTBEAT, then latches
`boot_banner_sent_ = true` forever. The flag is only re-armed on a
`DISCONNECTED` transition (3 s peer timeout while previously
`CONNECTED`). So:

- **Cold boot path (broken).** MCU boots, fires 10 banners into the
  void with no peer listening, latches. The bridge connects much later
  (after `pre_communication` + bringup time); MCU goes WAITING →
  CONNECTED but the latch is still set → no replay. Bridge times out
  on its 8 s grace and WARNs.
- **Daemon-restart path (intermittent).** The 3 s peer timeout *might*
  trigger before the new daemon comes up, re-arming the latch. But
  if the restart is faster than 3 s the latch stays sticky and we hit
  the same WARN.

Regex and STATUSTEXT plumbing are both correct:
`bridge/rosbot_mavlink_bridge/src/bridge_node.cpp:209` handles
`MAVLINK_MSG_ID_STATUSTEXT` and `:298` runs `std::regex_search` against
its text; firmware `src/rosbot_xl/mavlink_entities.cpp:67` emits exactly
`rosbot_xl <FW> mavlink` which matches `rosbot_xl .* mavlink`.

Verified on the current `jazzy` HEAD: the latch logic in
`lib/mavlink/mavlink_node.cpp` (case `WAITING` at :96, `emitBootBannerIfDue`
at :143-157, `case DISCONNECTED` reset at :119-122) is unchanged from
the v2.0.0-jazzy firmware that's currently vendored into `rosbot_utils/`
and flashed by `rosbot.flash`.

**Recommended fix (~3 lines, protocol-correct).** Reset the latch on
the WAITING → CONNECTED transition in `mavlink_node.cpp`, not only on
DISCONNECTED:

```cpp
case WAITING:
  if (peer_seen_) {
    boot_banner_sent_ = false;     // ← add
    boot_banner_attempts_ = 0;     // ← add
    state_ = CONNECTED;
  }
  break;
```

That guarantees every new peer (cold-boot or restart) sees the banner
stream from a fresh count of 0/10. The bridge's `peer_promoted_` flag
already idempotently absorbs a repeat banner on a long-lived
connection, so no bridge-side change needed.

**Alternatives considered.**
- **Drop the `kBannerAttempts` latch entirely**, emit at ~5 s cadence
  forever while WAITING/CONNECTED. Even simpler but spams STATUSTEXT.
- **Launch-side fix (rosbot_ros):** insert `reset_stm32` between
  `pre_communication` and the bridge in
  `rosbot_bringup/launch/mavlink.launch.py`. Zero firmware change but
  adds ~1–2 s to daemon startup and lives outside this repo.
- **Cosmetic only:** demote the bridge WARN to INFO. Hides the signal
  without fixing it — *not recommended*.

**Status.** OPEN 2026-05-30. Filed after end-to-end verification of the
flashing regression fix (`rosbot_ros@247b329`). Owner: TBD.
