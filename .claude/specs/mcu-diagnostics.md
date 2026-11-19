# Spec: diagnostyka MCU 1 Hz zamiast buildu debug

Status: zaimplementowane (commit 67f8d83), przetestowane na ROSbocie 3 z
wstrzykniętymi błędami, 2026-11-19. Pomysł: Rafał. Odstępstwa od szkicu:
progi są nazwanymi stałymi w `DiagnosticsMapper` (bridge nie używa
`generate_parameter_library`, nie mieszałem stylów), a wiadomość ma też
`fault_code` / `fault_pc` / `fault_task` (przyczyna ostatniego
`fatalError()`). IWDG nadal poza zakresem.

## Po co

Build debug istnieje dziś głównie po to, żeby task Monitor wypisywał na
FTDI obciążenie CPU, zapas stosów i stertę. To wymaga osobnej binarki,
kabla na tylnym USB i czytania tekstu ręcznie. Na robocie u klienta nikt
tego nie zobaczy, a błędy sprzętu (zawieszony czujnik, reset magistrali,
usterka sterownika silnika, reset MCU po błędzie) dziś albo giną, albo
wychodzą jako pojedyncza linia STATUSTEXT.

Cel: jeden firmware (release), który co 1 s wysyła stan sprzętu i
liczniki błędów, a bridge publikuje to w standardowym ROS
`/diagnostics` (`diagnostic_msgs/DiagnosticArray`), czytanym przez
`rqt_robot_monitor`, `diagnostic_aggregator` i Husarion Cockpit.

## Zakres

1. **Firmware: nowa wiadomość MAVLink `ROSBOT_DIAGNOSTICS`** (dialekt
   `rosbot.xml`, id 11030), 1 Hz, w każdym buildzie. Pola stałej długości:
   - `uptime_ms`, `reset_cause` (flagi z `RCC->CSR`: pin, POR/BOR,
     software, IWDG, WWDG, low-power + bit „fatalError” z markera w CCM),
   - `cpu_idle_permille` (z różnicy liczników run-time stats, jak dziś
     Monitor), `heap_free_bytes`, `heap_min_free_bytes`,
     `stack_min_free_bytes` i indeks taska z najmniejszym zapasem,
   - `imu`: status (ok / brak / błąd odczytu), liczba nieudanych odczytów,
   - `ranges[4]`: liczba timeoutów na czujnik + bit „zainicjalizowany”
     (ROSbot; XL wysyła zera),
   - `i2c_recoveries[2]`: resety magistrali (IMU, czujniki/EEPROM),
   - `motor_faults`: liczba zgłoszeń nFAULT + bit „aktywna teraz”,
   - `link`: ramki TX odrzucone przy pełnej kolejce, błędy parsowania RX
     (licznik MAVLink), liczba ponownych połączeń,
   - `battery_flags` (niski / krytyczny), na XL `power_board_ok`,
     `ethernet_link_up`.
   Liczniki są od startu (monotoniczne); interpretację „czy rośnie” robi
   bridge.
2. **Firmware: task Monitor zastąpiony przez publisher diagnostyki.**
   Zbieranie danych (statystyki tasków) przechodzi do małej biblioteki
   `lib/diagnostics/` (zbiera + koduje, bez wiedzy o MAVLink), publisher w
   `lib/mavlink/publishers/`. Moduły, które mają liczniki, wystawiają je
   getterami (już są: `I2c::recoveries()`, `RangeVl53l0x::timeouts()`).
   `#ifndef RELEASE` i tekstowy raport na FTDI znikają; preset `debug`
   zostaje tylko jako build z symbolami do gdb, z tym samym zachowaniem.
3. **Bridge: `ROSBOT_DIAGNOSTICS` → `/diagnostics`.** Jeden
   `DiagnosticArray` na wiadomość, statusy: `mcu` (CPU, sterta, stos,
   uptime, przyczyna resetu), `imu`, `ranges`, `motors`, `link`,
   `battery`/`power_board`. Poziomy OK/WARN/ERROR według progów (np. stos
   < 64 B → ERROR, idle < 20 % → WARN, licznik timeoutów rósł w ostatniej
   minucie → WARN, reset przez fatalError → ERROR do restartu bridge'a).
   Progi w `config/rosbot*.yaml`.
4. **ROS_API.md** w tym samym commicie co bridge (nowy topik).

## Poza zakresem

- Watchdog sprzętowy (IWDG): przydatny, ale to osobna zmiana z testem na
  HW (reset przy zawieszeniu taska). W diagnostyce jest już bit IWDG w
  przyczynie resetu, więc dodanie go później nie zmienia formatu.
- Zmiany w Cockpit.

## Ryzyka

- Pasmo: wiadomość ~80 B co 1 s to <0,1 % łącza 921600.
- Kompatybilność: starszy bridge ignoruje nieznane id wiadomości; nowy
  bridge ze starym firmware po prostu nie publikuje `/diagnostics`
  (można dodać status „brak danych z MCU” po 5 s).
- Dialekt: regeneracja nagłówków (`just mavgen`) i job CI
  `verify-dialect`.

## Testy

- Host (CTest): kodowanie/progi w `lib/diagnostics/` (CPU z liczników,
  wybór najmniejszego stosu, flagi resetu z rejestru) na fake'ach.
- Bridge (gtest): mapowanie wiadomości → `DiagnosticArray`, progi,
  „licznik rośnie” → WARN.
- HW na ROSbocie 3 przez tylny USB z wymuszonymi błędami (flagi tylko w
  kopii testowej): brak IMU (bez zasilania IMU), zawieszony czujnik
  (XSHUT w dół w czasie pracy), reset magistrali I2C, usterka silnika
  (wymuszony nFAULT), niska bateria (podmieniony odczyt), mały zapas stosu
  (zjedzenie stosu w tasku), reset po `fatalError()` (komenda testowa),
  zalanie TX (gubione ramki), śmieci na RX (błędy parsowania). Dla każdego
  sprawdzić odpowiedni status i poziom w `/diagnostics`.
