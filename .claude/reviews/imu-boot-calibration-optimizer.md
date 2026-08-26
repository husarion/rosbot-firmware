# Optimizer review — imu-boot-calibration vs jazzy

Overall: clean, well-scoped diff. `boot_option` is a real, justified
extraction (used by both variants, replaces two near-duplicate
useAlt()/confirmAlt() pairs). `init()`/`enableDmaReads()` split is a
correct fix for a real HW-verified bug, documented in both code and
ARCHITECTURE.md. No premature abstractions, no unused params, no
speculative generality. Findings below are all minor/doc-level.

| Severity | Location | Finding |
|---|---|---|
| suggest-remove | `lib/imu/imu_bno055.{hpp,cpp}` `probeCalibStatRaw()` | Diagnostic added during debugging of the DMA/Wire conflict. Now runs unconditionally, once per second, for the entire boot-calibration window whenever `debug_serial` is non-null (i.e. every calibration boot with a debug line attached) — not gated behind any "verbose" flag. It's still defensible as a permanent tool (it catches exactly the class of bug — `read8()` silently returning the register address on I2C failure — that motivated the whole `enableDmaReads()` split), but it's public API on `ImuBno055` for a single call site inside `imu_calibration_boot.cpp`. Recommend either: (a) keep as-is (acceptable, HW-verified real bug class, cheap — only runs during the boot window), or (b) if kept, don't expose it as a public class method — inline it as a static helper in `imu_calibration_boot.cpp` using `cfg_`-equivalent access, since nothing else will ever call it. Not urgent either way. |
| suggest-remove | `src/rosbot/ros.cpp:174`, `src/rosbot_xl/ros.cpp:218` | Comment says "see `runImuCalibrationWindow()` in main.cpp" — no such function exists; the actual function is `imu_calibration_boot::run()`. Stale/renamed reference, both files. |
| keep-but-note | `lib/boot_option/boot_option.hpp:29` | Doc comment references `kConfig.calibration_hold_ms` — no `kConfig` symbol exists (it's `cfg.calibration_hold_ms`, a runtime field, not a constant). Cosmetic, low-confusion risk since the struct is right below. |
| keep-but-note | `src/rosbot/ros.cpp:177`, `src/rosbot_xl/ros.cpp:221` | `imu_calibration_status_service` (an `rcl_service_t`) is declared but never referenced elsewhere in the file — `ServiceEntry.srv` is `{}`, not `&imu_calibration_status_service`. This mirrors the pre-existing `mcu_id_service` pattern in the same file (also unused), so it's consistent with existing convention rather than a new defect — not worth fixing in isolation, but if `mcu_id_service` is ever cleaned up, this one should go with it. |

No findings on: `lib/persistent_config/*` (record layout change is a clean additive field, save()'s early-out correctly extended to cover the new fields), `lib/imu/imu_calibration_boot.cpp` (all of its dependencies, including `probeCalibStatRaw`, are actually used — nothing dead there), `src/rosbot*/main.cpp` (the two variants' `setup()` changes are structurally parallel but not mechanically identical — rosbot has 2 buttons + 2 LEDs, xl has 1 of each — a shared helper would need to take most of these as parameters anyway, so the current duplication is reasonable, not copy-paste debt).
