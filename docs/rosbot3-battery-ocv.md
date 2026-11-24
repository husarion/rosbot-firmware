# ROSbot 3 battery: open-circuit voltage curve

Source of `ocv_curves::kRosbot3` in `lib/battery/soc_estimator.hpp`.

## Setup

- Pack: ROSbot 3 stock pack, 3S1P 18650 Li-ion, 3500 mAh nominal, taken out
  of the robot. Measured 2026-11-23, room temperature.
- Discharger: Redox Beta, constant-current discharge from full, in steps,
  with the pack resting between steps before the voltage reading.
- The Redox Beta caps discharge at 5 W (0.1-1.0 A). It displays the set
  current (0.7 A), but at 10-12 V the real current was 5 W / V_load,
  0.42-0.52 A. Charge per step is therefore computed as
  `min(set current, 5 W / V_load) x time`, with V_load = mean rest voltage
  of the step minus 0.2 V. Counting the set 0.7 A instead gives 5.2 Ah,
  which a 3S1P 18650 pack cannot hold; the 5 W correction gives 3.54 Ah,
  matching the nominal 3.5 Ah.

## Readings

Pack voltage at rest; SoC = 1 - removed / 3543 mAh.

| Step | Discharge | Rest [min] | Removed [mAh] | Pack [V] | Cell [V] | SoC |
|---|---|---|---|---|---|---|
| 0 | - | - | 0 | 12.60 (assumed) | 4.200 | 1.000 |
| 1 | 53 min at 0.4 A | ~30 | 353 | 12.27 | 4.090 | 0.900 |
| 2 | 2 min at 0.3 A + 29 min | ~15 | 565 | 12.15 | 4.050 | 0.841 |
| 3 | 31 min | ? | 783 | 11.95 | 3.983 | 0.779 |
| 4 | 30 min | 30 | 997 | 11.79 | 3.930 | 0.719 |
| 5 | 30 min | 30 | 1214 | 11.62 | 3.873 | 0.657 |
| 6 | 30 min | 30 | 1434 | 11.47 | 3.823 | 0.595 |
| 7 | 30 min | >30 | 1658 | 11.26 | 3.753 | 0.532 |
| 8 | 30 min | ? | 1886 | 11.10 | 3.700 | 0.468 |
| 9 | 30 min | ? | 2117 | 10.97 | 3.657 | 0.403 |
| 10 | 30 min | ? | 2350 | 10.84 | 3.613 | 0.337 |
| 11 | 29 min | 10-20 | 2579 | 10.70 | 3.567 | 0.272 |
| 12 | 30 min | 10-20 | 2820 | 10.47 | 3.490 | 0.204 |
| 13 | 15 min | 10-20 | 2942 | 10.37 | 3.457 | 0.170 |
| 14 | 20 min | 10-20 | 3107 | 10.29 | 3.430 | 0.123 |
| 15 | 22.5 min | 25 | 3294 | 10.10 | 3.367 | 0.070 |
| 16 | 14 min | 10 | 3413 | 9.90 | 3.300 | 0.037 |
| 17 | 15 min | 60 | 3543 | 9.75 | 3.250 | 0.000 |

Steps 2-17 ran at a set 0.7 A (5 W capped). At the end of step 17 the pack
read 9.41 V under load, below the 9.6 V (3.2 V/cell) cut-off, so that point
is 0 %. Load sag there was 0.32 V at ~0.52 A (~0.6 ohm, including
polarisation).

The firmware curve resamples these points at 0, 5, 10, 20 ... 100 % by linear
interpolation.

## Limits

- 100 % (4.20 V/cell) was not measured after the charge.
- Rests of 10-20 min (steps 11-16) read a few tens of mV low; the knee may
  sit slightly higher than in the table.
- The current is derived from the 5 W cap, not measured. A scale error moves
  the capacity but not the curve shape in SoC.
- Only the pack voltage was read, not single cells.
