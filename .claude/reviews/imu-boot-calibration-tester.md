# tester review — imu-boot-calibration (vs jazzy)

No automated test suite exists in this repo (no PlatformIO `test/`, no unit
framework). Build verification + manual code-path/QA review only, per the
tester's task briefing.

## Build verification

`pio run -e rosbot -e rosbot_xl -e rosbot_release -e rosbot_xl_release` —
**all 4 SUCCESS**, ~15s total, no warnings surfaced in captured output.

| env | result | RAM | Flash |
|---|---|---|---|
| rosbot | SUCCESS | ~72% | ~23% |
| rosbot_release | SUCCESS | — | — |
| rosbot_xl | SUCCESS | 72.5% | 23.0% |
| rosbot_xl_release | SUCCESS | 72.5% | 23.0% |

## Findings

| # | Severity | File:Line | Finding |
|---|---|---|---|
| 1 | **Critical** | `src/rosbot/ros.cpp:186` (`imuCalibrationStatusCallback`), `src/rosbot_xl/ros.cpp:226` (same) | New `_imu_calibration_status` ROS service calls `g_imu_bno055->getCalibrationStatus()`, which uses `bno_.getCalibration()` — a **blocking Wire (I2C) call**. `lib/imu/imu_bno055.cpp`'s own doc comment on `enableDmaReads()` states, HW-verified: *"raw I2C reads succeed before this call, fail with a generic HAL error immediately after"* once DMA is enabled. Both `main.cpp`s call `enableDmaReads()` before `vTaskStartScheduler()`, and the ROS service is only reachable once the node/executor is running — i.e. **always after** DMA is enabled. The service is therefore broken by construction: every call happens exactly in the window the code's own comment says blocking I2C fails. ROS_API.md documents this as a live, working service — it isn't. Needs either a DMA-based calibration-status read path, or the service needs to be reworked/removed. |
| 2 | **High** | `lib/imu/imu_bno055.cpp` (`s_hi2c`, DMA state) + new `_imu_calibration_status` service | Unrelated to #1 even if the blocking call somehow returned instead of erroring: `update()` (DMA, called periodically from the IMU RTOS task) and `getCalibrationStatus()`/`probeCalibStatRaw()` (blocking Wire, called from the ROS executor task via the new service) touch the same `s_hi2c`/bus with **no mutex or synchronization**. Two RTOS tasks issuing concurrent I2C transactions on the same peripheral is a race independent of whether blocking reads currently "work" after DMA init — worth flagging even if #1 is fixed by another means. |
| 3 | **High** | `src/rosbot/main.cpp:170` | `if (!imu_ready && g_comm_mgr.hasDebugSerial())` — De Morgan slip. Intent (per `rosbot_xl/main.cpp:214`'s simpler `if (imu_ready) { ... }` sibling, which has no bug) is clearly "skip calibration logic when the IMU isn't ready." As written, when `imu_ready == false` **and** no debug serial is attached, control falls into the `else` branch and runs `applyCalibrationOffsets()` / `imu_calibration_boot::run()` against a BNO055 that failed `init()`. Concrete impact on rosbot (not rosbot_xl, which doesn't have this bug): a unit with no debug UART connected, whose BNO055 didn't come up (bad solder joint, cold-boot glitch, unseated sensor), that also has the calibration boot gesture held, will block boot for up to `timeout_ms` (default 120 s) spinning on failing I2C reads instead of failing fast. Should be `if (!imu_ready) { if (has debug serial) print; } else { ...calibration... }`. |
| 4 | **Medium** | `lib/persistent_config/persistent_config.cpp` `Record` struct / `load()` | Backward-compat trap, as suspected. Old `Record` layout: `magic(4)+backend(1)+pad[3](3)+ns[32]+crc(4)` = 44 B, with `crc` computed over the first 40 bytes. New layout inserts `has_imu_calibration` + `imu_calibration` **before** `crc`, so `offsetof(Record, crc)` grows and the field that used to be `crc` (bytes 40-43) is now read as the *start* of `imu_calibration`, while the real `crc` field moves out into what was previously untouched (erased, `0xFF`) flash beyond the old record. On firmware upgrade, a unit that already has a valid old-format record in sector 11 will: (a) match `magic` (unchanged, offset 0), but (b) almost certainly fail the new CRC check, because `expected` is now computed over old-crc-bytes-reinterpreted-as-calibration-data plus erased filler, compared against `stored->crc` read from erased flash (`0xFFFFFFFF`). Net effect: **any already-deployed unit with a saved comm-backend/namespace choice silently reverts to defaults (MAVLINK, empty namespace) on first boot after this firmware update** — not a crash, not memory-unsafe (sector 11 is fully reserved flash, so the extra read stays in-bounds), but a silent config loss that field-deployed robots would hit. Worth an explicit decision: is this acceptable ("format bump == fresh start", document it in a migration note / CHANGELOG) or does it need a real magic/version field so old records can still be read for the fields that didn't move? Currently nothing in ARCHITECTURE.md/CHANGELOG.md calls this out as intentional. |
| 5 | none (confirmed correct) | `lib/imu/imu_bno055.cpp` `init()`/`enableDmaReads()` split, both `main.cpp`s | Verified every call site: `init()` → (apply saved calibration / run boot calibration, both need blocking I2C) → `persistent_config::save()` → backend/link setup → `enableDmaReads()` (only if `imu_ready`) → `createTasks()`/`vTaskStartScheduler()`. Ordering matches the documented contract in both `rosbot` and `rosbot_xl` `main.cpp`. No call to `update()` before `enableDmaReads()` — the only caller of `update()` is the IMU task created in `createTasks()`, which cannot run before `vTaskStartScheduler()`. `s_done_sem == nullptr` guard makes a hypothetical pre-`enableDmaReads()` `update()` call a safe no-op regardless. |
| 6 | none (looks correct) | `lib/boot_option/boot_option.cpp` `resolveBootAction()` | Press-detect window and hold-threshold timing logic checked for off-by-one / boundary issues: uses `millis() - start >= threshold` (correct wraparound-safe unsigned subtraction pattern, handles the ~49-day `millis()` rollover). Button-already-held-at-entry case exits the detect loop immediately (no wasted `delay(10)`). Threshold-crossing is detected on the hold loop's own poll cadence (±10 ms jitter vs `calibration_hold_ms`) — acceptable given `confirmCalibrationEntry()` doesn't wait for release, matches doc comment. No bug found. |

## Coverage gaps needing HW-in-the-loop (flagged, not fixed)

All of the following are new logic with **zero automated coverage** (none possible without a test harness) and should be exercised on real hardware before merge:

- `resolveBootAction()`'s three timing branches (no press, tap-release, hold-to-calibrate) on both rosbot (2-button OR) and rosbot_xl (1-button) — especially the boundary right at `calibration_hold_ms`.
- `imu_calibration_boot::run()` end-to-end: real BNO055 reaching `system==3` etc., LED blink/solid transitions, timeout path with a sensor that never calibrates.
- The persistent_config migration trap (finding #4) — needs a physical unit flashed with the pre-PR firmware, given a non-default backend/namespace, then reflashed with this branch, to observe the actual silent-revert-to-defaults behavior.
- The DMA/blocking-I2C conflict (findings #1, #2) — needs a live micro-ROS session calling `_imu_calibration_status` after boot to observe the actual failure mode (error response vs. hang vs. corrupted IMU data momentarily from the concurrent access).
- `enableDmaReads()`'s DMA/IRQ wiring itself (stream/channel selection per I2C instance, NVIC priority interaction with Wire's own IRQs) — inherently unverifiable off real silicon.

## Would current CI have caught this?

No CI workflow file was inspected as part of this review scope, but structurally: none of findings #1-#4 are compile-time detectable (no `-Wall`/`-Wextra`/static-analysis finding would flag a blocking-vs-DMA I2C semantic conflict, a struct-layout migration trap, or a boolean logic inversion that still type-checks). The only thing CI-equivalent tooling here (`pio run`) would catch is a build break, and the build is clean. This class of bug requires either HW-in-the-loop testing or a host-side unit test harness (e.g. extracting `resolveBootAction()`'s timing logic and the `Record`/`load()`/`save()` logic behind a mockable clock/flash interface) — neither exists today.
