# Spec: taktowanie MCU z kwarcu (HSE) zamiast z wewnętrznego RC (HSI)

Status: zaimplementowane i przetestowane na HW (oba roboty), 2026-11-20.
Ustalenie 14 z planu migracji. Odstępstwo od szkicu: zamiast całego HAL per
płytka budowane są per płytka tylko trzy pliki czytające `HSE_VALUE`
(`system_stm32f4xx.c`, `stm32f4xx_hal_rcc*.c`) i `system.cpp`; źródło zegara
w diagnostyce czytane wprost z `RCC_PLLCFGR` (PLLSRC).

Wyniki HW: XL −9 ppm względem hosta (DWT, 2 min), 10 min bez korekty
TIMESYNC (było 17), przerwy w znacznikach dokładnie 10 / 5 ms; ROSbot 3
5 min bez korekty, IMU 100,0 Hz i feedback 200,0 Hz (na HSI 99,8 / 199,6);
fallback (HSE w bypass bez zegara) → start na HSI, MCU WARN w
`/diagnostics`.

## Po co

Oba firmware (jazzy i nowy) taktują STM32F407 z HSI 16 MHz → PLL 168 MHz.
HSI ma ±1 % w zakresie temperatur. Zmierzone na HW 2026-11-20:

- dryf zegara MCU względem hosta ~1500–2000 ppm;
- na XL bridge co 30–40 s przesuwa offset TIMESYNC o ~57 ms („offset
  stepped”), a znaczniki `/imu` i `/_motors/feedback` skaczą wtedy ~57 ms
  do przodu (10 korekt w 5 min w jazzy, 17 w 10 min w nowym);
- tę samą tolerancję dziedziczą baud rate'y (link SBC 921600, UART power
  boardu) i każdy timer (PWM silników, wentylatora, 1 kHz tick).

Z kwarcem dryf spada do kilkudziesięciu ppm: korekty TIMESYNC znikają, a
znaczniki czasu są ciągłe.

## Fakty sprawdzone na HW

| Płytka | Kwarc (schemat) | Pomiar 2026-11-20 |
|---|---|---|
| ROSbot XL Digital Board v1.2 (i v1.1) | Q1 24 MHz na PH0/PH1 | HSE startuje (HSERDY), SYSCLK=HSE przez SWD: 48,02 mln cykli w ~2,0 s → 24 MHz |
| CORE2 v1.3+ (ROSbot 3) | Q1 na PH0/PH1, wartość niepodpisana | firmware testowy: TIM11 łapie HSE/16, 2682 tiki na 8 okresów przy 168 MHz z HSI → 8,02 MHz (±1 %) |

Starszych rewizji CORE2 (< 1.3) nie sprawdzałem; fallback niżej je chroni.

## Zakres

1. **`systemClockConfig()` (`boards/stm32f407/system.cpp`)**: najpierw
   HSE → PLL, a gdy HSE nie wstanie w `HSE_STARTUP_TIMEOUT` (100 ms),
   dotychczasowa konfiguracja HSI. Wyjście identyczne jak dziś (SYSCLK 168,
   APB1 42, APB2 84 MHz, PLLQ 48 MHz), więc żaden timer, UART ani ETH nie
   zmienia ustawień:
   - CORE2 8 MHz: PLLM 4, PLLN 168, PLLP 2, PLLQ 7 (wejście VCO 2 MHz,
     zalecane przez RM0090 dla mniejszego jittera);
   - XL 24 MHz: PLLM 12, PLLN 168, PLLP 2, PLLQ 7.
   Wartości per płytka z `HSE_VALUE` (`PLLM = HSE_VALUE / 2 MHz`).
2. **`HSE_VALUE` per płytka.** HAL liczy z niego `SystemCoreClock`, a z
   tego baud rate'y i tick, więc musi być prawdziwe. Dziś HAL jest jedną
   biblioteką dla obu płytek z `HSE_VALUE` 8 MHz w `stm32f4xx_hal_conf.h`.
   Propozycja: `stm32f4_hal` budowane osobno dla każdej płytki
   (`stm32f4_hal_rosbot`, `stm32f4_hal_rosbot_xl`) z
   `HSE_VALUE=8000000` / `24000000` w definicjach kompilacji; `fw_common`
   nie używa `HSE_VALUE`, zostaje jeden. Koszt: HAL kompiluje się dwa
   razy (kilkanaście sekund czystego buildu).
3. **Clock security system (CSS).** Włączone po przejściu na HSE: gdy
   kwarc stanie w trakcie pracy, sprzęt sam przełącza na HSI i zgłasza
   NMI. `NMI_Handler` przy fladze CSSF robi `fatalErrorWithCode(kFaultClock)`
   zamiast `kFaultNmi`; po resecie start i tak spadnie na HSI, jeśli kwarc
   nie wstanie.
4. **Diagnostyka.** Nowe pole rozszerzające w `ROSBOT_DIAGNOSTICS`
   (`clock_source` uint8: 0 nieznane / stary firmware, 1 kwarc, 2 RC po
   nieudanym starcie kwarcu). MAVLink 2 extension: stary bridge go nie
   widzi, nowy bridge ze starym firmware dostaje 0. Bridge: status MCU
   WARN „Crystal did not start, running on the internal oscillator” przy 2,
   pole „Clock source” w wartościach. `ROS_API.md` w tym samym commicie.

## Poza zakresem

- Modelowanie dryfu w `TimesyncEstimator` (po zmianie zbędne).
- Zmiana progu 50 ms re-seedu TIMESYNC.

## Ryzyka

- **Rewizja bez kwarcu albo z innym kwarcem** (stare CORE2): bez HSERDY
  start zostaje na HSI jak dziś; inna częstotliwość kwarcu dałaby zły zegar,
  czego fallback nie wykryje. Wiem tylko o v1.3+ i DB v1.1/v1.2.
- **Start dłuższy o czas wstawania kwarcu** (typowo 1–2 ms, maks. 100 ms
  przy braku kwarcu).
- **Baud rate'y zmienią się minimalnie** (dziś są przesunięte o błąd HSI,
  po zmianie dokładne); nic po drugiej stronie nie polega na tym błędzie.
- **CSS + NMI**: fałszywe wyzwolenie zresetowałoby MCU; w F407 CSS reaguje
  tylko na zatrzymanie HSE.
- Zmiana core flow (zegar) → obowiązkowo HW na obu płytkach.

## Testy

- Build obu płytek, testy hosta (bez zmian w logice; mapper diagnostyki
  dostaje test `clock_source`).
- XL przez SWD: RCC_CFGR SWS = PLL, RCC_PLLCFGR PLLSRC = HSE, PLLM 12;
  DWT przeciw czasowi hosta → 168 MHz z dokładnością do ppm.
- Obie płytki: 10 min z bridge'em → 0 korekt TIMESYNC (dziś co 30–40 s),
  dryf `clock_drift_ms` w ppm; link bez błędów CRC, `/battery` z power
  boardu, PWM wentylatora 25 kHz (XL), silniki i watchdog jak dziś.
- Fallback: obraz testowy z `HSEBYP` bez zewnętrznego zegara (HSERDY nigdy
  nie przyjdzie) → start na HSI, `clock_source` = 2, WARN w `/diagnostics`.
- Porównanie z wynikami z 2026-11-19/20 (CPU, opóźnienia) — bez zmian
  oczekiwanych, bo częstotliwości są te same.
