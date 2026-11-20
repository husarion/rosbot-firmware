# Plan: Arduino → STM32Cube HAL + FreeRTOS + ETL

Status: kroki 0–7 na lokalnym `devel`, kroki 8–10, poprawki z testów HW,
optymalizacje i diagnostyka na lokalnym `cube-switchover`, 2026-11-19.
ROSbot 3 (CORE2) przetestowany zdalnie przez tylny USB, ROSbot XL
(Digital Board v1.2) przez Ethernet i ST-LINK, 2026-11-20 (wyniki niżej).

Decyzje (Rafał, 2026-11-19): CMake + CubeMX po kroku 9; kroki 8–9 na
krótko żyjącym branchu, merge do `devel` po teście HW na obu robotach;
ETL tylko tam, gdzie zastępuje kod pisany ręcznie.

## Cel

Pozbyć się `framework = arduino` (stm32duino + fork Husarion, biblioteki
Adafruit/Pololu, wrappery STM32Ethernet/LwIP). Firmware działa na
startupie + STM32CubeF4 HAL/LL, z jądrem FreeRTOS używanym bezpośrednio
(bez CMSIS-RTOS) i ETL dla kontenerów o stałej pojemności. Bez wyboru
wariantu przez preprocesor.

## Sprawdzone ograniczenie: HAL2 nie istnieje dla STM32F4

Tabela oferty ST (github.com/STMicroelectronics/STM32Cube_MCU_Overall_Offer,
sprawdzone 2026-11-19) wymienia HAL2 tylko dla STM32C5, STM32G5 i STM32H7P.
STM32F4 jest w tabeli HAL1, a pracownicy ST napisali, że nie planują
portowania starszych serii. Cel to więc **STM32CubeF4 (HAL1) + LL**, z LL
w gorących ścieżkach (enkoder, PWM, UART DMA). Kod jest pisany tak, żeby
późniejszy port na HAL2 dotykał tylko `lib/platform/`.

## Ustalenia (co poprawić poza samym frameworkiem)

Rozwiązane przez odejście od Arduino (opisane w ARCHITECTURE, „Open work”):

1. RX szeregowy na rosbocie to polling z `vTaskDelay(1)`:
   `HardwareSerial::_serial` jest prywatne, a `HAL_UART_RxCpltCallback` to
   silny symbol w core. Własny sterownik UART: kołowy RX przez DMA +
   przerwanie IDLE + powiadomienie taska.
2. `HAL_I2C_ErrorCallback` jest silny w core, więc ścieżka DMA BNO055 nie
   może zgłaszać błędów i polega na timeoucie.
3. `Wire` przy `begin()` resetuje magistralę do 100 kHz (pułapka „Wire
   100 kHz reset”, `lib/i2c_fast_mode`).
4. Fork core Arduino od Husarion trzeba utrzymywać tylko dla rozmiarów
   buforów UART i mapy pinów RMII.
5. Buildy release są w praktyce `-Os`: platforma dokleja swoją flagę po
   naszej. Z własnym CMake sami wybieramy (i mierzymy) poziom.
6. Kolizje symboli handlerów przerwań USART z core (ARCHITECTURE,
   „USART → DMA mapping”).

Do poprawienia w trakcie migracji:

7. Użycia sterty po starcie da się uniknąć: `new HardwareTimer` w
   `motor_hi_z.cpp`, `xTaskCreate`/`xQueueCreate` z heap_4. Cel:
   `configSUPPORT_DYNAMIC_ALLOCATION 0`, statyczne taski i kolejki.
8. ~~Częstotliwość taska przechodzi przez `void*` taska jako
   `float → uintptr_t → uint16_t`, więc ułamkowa częstotliwość się
   obcina.~~ Poprawione w kroku 1.
9. Zmienne globalne używane z `lib/` (`g_battery`, `g_imu`, `g_comm_mgr`,
   `g_motors`, `g_indicator`) wiążą biblioteki z aplikacją. Wstrzykiwać je.
10. `src/rosbot/rtos.cpp` i `src/rosbot_xl/rtos.cpp` powielają większość
    tasków (~190 różnych linii z ~500). Wspólne taski idą do wspólnego
    pliku; płytki różnią się tylko tabelą tasków i obiektami płytki.
11. Wspólna magistrala I2C2 (XL: DMA IMU + EEPROM) nie ma mutexa;
    bezpieczne tylko dlatego, że EEPROM jest używany wyłącznie przy
    starcie. Nowy sterownik I2C dostaje mutex.
12. `#ifndef RELEASE` dla taska Monitor: zamienić na opcję CMake / stałą
    zależną od typu buildu (`constexpr bool kDebugBuild`).
13. Repo-owy `CLAUDE.md` był nieaktualny: branch roboczy `jazzy`,
    „user converses in English”, trailer co-author Opus 4.7. Poprawione
    przy kroku 10.
14. **MCU chodzi na wewnętrznym oscylatorze RC, nie na kwarcu.**
    `SystemClock_Config` generycznego wariantu F407ZG z Arduino wybiera
    `RCC_OSCILLATORTYPE_HSI` (±1 % w zakresie temperatur wg datasheetu) →
    PLL 168 MHz. Każdy baud rate (link SBC 921600, UART power boardu) i
    każdy timer dziedziczą tę tolerancję. Sprawdzić na schematach, czy jest
    kwarc HSE; jeśli tak, użyć go (zachowane HSI w kroku 9, bez zmian).
15. ~~Skala żyroskopu BNO055.~~ To nie problem: w Adafruit_BNO055 1.6.4
    zapis UNIT_SEL jest zakomentowany, więc chip zawsze chodził na
    wartości po resecie 0x80 (żyroskop 1/16 deg/s), i to właśnie przelicza
    `ImuBno055::update()`. Sterownik z kroku 7 przez chwilę zapisywał
    UNIT_SEL = rad/s (znalezione w review przed testem HW; `/imu`
    angular_velocity byłoby ~1,8 % za niskie, a zapisane offsety żyroskopu
    przeskalowane); tego zapisu znów nie ma.

## Model wariantów bez `#ifdef`

Zrobione w kroku 0: nie ma makra `ROSBOT`/`ROSBOT_XL`, dispatchera ani
wariantowego `#ifdef` w `lib/`.

Cel: jeden plik wykonywalny CMake na płytkę (`rosbot`, `rosbot_xl`), każdy
linkuje wspólne obiekty `lib/` i swoje `src/<variant>/`. Wybór przez
linkowanie, nigdy przez makro.

Jedna binarka z wykrywaniem w runtime została rozważona i odrzucona:
płytki używają innych peryferiów (Ethernet + LAN9303 vs link UART, bateria
z ADC vs power board), a XL już zajmuje 41 % statycznego RAM.

## Fazy

Każdy krok zostawia wszystkie warianty budujące się i testy hosta
zielone, więc `devel` da się flashować. Kroki oznaczone HW wymagają
uruchomienia na obu robotach, zanim wejdzie następny.

| # | Krok | Ryzyko | HW |
|---|------|--------|----|
| 0 | Usunięcie makr wariantów + dispatchera `config.hpp` | brak (kod maszynowy identyczny, sprawdzone) | nie |
| 1 | ETL (przypięty) z pierwszymi użytkownikami (`etl::span` w `MotorArray`/`RangeArray`), `platform::Pin` + testy hosta, taski dostają swój `TaskConfig*` (ustalenie 8). Czas przeniesiony do kroku 3, statyczne helpery RTOS do kroku 9, gdzie są ich pierwsi użytkownicy | niskie | nie |
| 2 | GPIO: `pinMode/digitalWrite/digitalRead` → `platform::Pin` (`Pin{Port::E, 2}` w konfiguracjach płytek). Zrobione dla każdego czystego pinu GPIO; piny zasilające też ADC lub PWM (bateria, NTC, pomiar prądu, wentylator) zostają na Arduino do kroków 4–5 | niskie | tak |
| 3 | Czas: `platform::clock` na TIM5 (32-bit, 1 MHz) zasila `time_boot_us()`; `millis()` → `time_boot_ms()`, `micros()` → `clock::counterUs()`, `delay()` → `clock::delayMs()` (`vTaskDelay` po starcie schedulera, wcześniej aktywne czekanie). Biblioteki Arduino (Adafruit, Pololu, Wire) zostają na zegarze core do kroku 7 | średnie (niezmienniki time_base, TIMESYNC) | tak |
| 4 | `platform/timer`: wspólne włączanie zegara timera / zegar jądra, `PwmOutput` (rejestry, te same PSC/ARR/PWM1/preload co `HardwareTimer`, testy hosta) dla silników; enkodery i wentylator zostają przy kodzie timerów HAL, ale piny dostają przez `Pin::setAsAlternate` | średnie (sterowanie silnikami) | tak |
| 5 | `platform::AnalogIn`: pojedyncza konwersja ADC1 na rejestrach z tą samą konfiguracją co `analogRead()` (10 bit, 15 cykli, PCLK2/4), konwersja przy zamaskowanych przerwaniach (taski silników i wentylatora na XL dzielą ADC1). Bez DMA: przy obecnych częstotliwościach nic go nie potrzebuje | niskie | tak |
| 6 | `platform::Uart`: rejestry + kolejka TX przez DMA + kołowy RX przez DMA dla UART-ów linku, debugu i power boardu; `HardwareSerial` i `-D ENABLE_HWSERIALx` usunięte. Ustalenie 1 zamknięte w połowie: nie ma przerwania na każdy bajt RX, ale wybudzanie przez IDLE czeka na krok 9 (`USARTx_IRQHandler` jest silny w `uart.o` core, który `Print.cpp` i Adafruit zawsze linkują) | wysokie (link) | tak |
| 7 | `platform::I2c` (HAL, mutex magistrali, odczyt przez DMA, własne przerwania/callbacki), własny sterownik BNO055, dołączony Pololu VL53L0X na nim, EEPROM; `platform::Spi` dla taśmy LED; żaden plik nie dołącza już `Arduino.h`. Wire, Adafruit (BNO055/BusIO/Sensor) i Pololu wypadają z linkowania. Zamyka ustalenia 2, 3, 11 | wysokie (IMU) | tak |
| 8 | `lib/ethernet`: dołączony `ethernetif` (ST + LAN9303 od Husarion) + nasze uruchamianie (statyczne IP, DHCP inform, task lwIP, sprawdzanie linku) zastępują STM32Ethernet; `include/lwipopts.h` ma te same wartości opcji. HAL ETH zostaje w API sprzed 1.27 z core; przejście na nowe API ETH F4 to osobna zmiana po kroku 9 | wysokie | tak |
| 9 | Przełączenie: CMake (`cmake/`), CubeF4 HAL 1.8.1 + CMSIS device 2.6.8 + FreeRTOS-Kernel 10.3.1 + lwIP 2.2.1 przypięte przez FetchContent (wersje, które linkował build Arduino), `boards/stm32f407/` (startup, linker script z zarezerwowanym sektorem 11 i nietkniętym CCM, hal_conf z sekcją ETH dla LAN9303, FreeRTOSConfig, zegar/SysTick/main, sterta newlib, syscalle); `framework = arduino` usunięty. Bez `.ioc` i bez alokacji statycznej, patrz uwagi | wysokie | tak |
| 10 | Narzędzia: CI, `justfile`, `scripts/build.sh` + `flash.sh`, `release.yaml` (nazwy artefaktów bez zmian), taski VS Code, testy hosta → CTest + Unity 2.6.1, `platformio.ini` usunięty; sekcja „Build” w ARCHITECTURE przepisana | średnie | nie |

Kroki 2–8 działają wewnątrz buildu Arduino, na HAL, który core i tak
linkuje. O to chodzi: każdy sterownik jest wymieniany i testowany na HW
osobno, zamiast jednego dużego przełączenia, w którym zmienia się
wszystko naraz. Koszt: do kroku 9 nasze sterowniki muszą omijać kolizje z
handlerami przerwań i słabymi callbackami core (ten sam problem co w
ustaleniach 1, 2, 6), więc kilka sterowników staje się w pełni
przerwaniowych dopiero w kroku 9.

## Uwagi do kolejnych kroków

- `Pin` zachowuje bardzo wysoką prędkość wyjść z Arduino dla zgodności;
  jej obniżenie (EMI) to osobna zmiana z pomiarem na HW.
- `MotorArray` przycina teraz do `MAX_NUM_MOTORS` (wcześniej przepełniał
  `MotorsData` przy więcej niż 4 silnikach; obie płytki mają dokładnie 4).
- `pin.hpp` wybierał `stm32_def.h` przez `__has_include`, dopóki core
  Arduino był linkowany (nagłówki core psują się, gdy CMSIS jest pierwszy).
  Usunięte w kroku 9.
- Konfiguracje płytek używają `using platform::Pin/Port` w zasięgu
  globalnym; przeniesienie do przestrzeni nazw płytki zostaje na później.
- Różnice zachowania z kroku 2 do obserwacji na HW: wyjścia dostają swój
  poziom, zanim pin stanie się wyjściem (Arduino najpierw przełączało
  tryb, więc np. Brake przez chwilę podawał stary poziom);
  `MotorHiZ::setMode` jest dużo tańszy niż `pinMode()` (bez szukania w
  mapie pinów), więc task sterowania się skraca.
- **Błąd z kroku 2 znaleziony na HW (2026-11-19, poprawiony w `3a357b8`):**
  `setAsOutput(true)` zapisywał BSRR, zanim `configure()` włączył zegar
  portu, więc pierwszy pin na porcie startował nisko. Na ROSbocie 3 pin
  adresu BNO055 (PC8, pierwszy użytkownik GPIOC) był niski i IMU
  odpowiadało pod 0x28 zamiast 0x29 („IMU init failed: BNO055 not
  found”). Teraz zegar portu włącza się przed zapisem poziomu.
- Różnice zachowania z kroku 3: `delay()` w core to czyste aktywne
  czekanie (STM32FreeRTOS nie nadpisuje `yield()`), więc `delayMs()` w
  tasku oddaje teraz CPU (`ImuBno055::captureCalibrationOffsets` z
  `imuTask` to jedyne wywołanie w runtime). `time_boot_us()` liczy od
  `platform::clock::init()` (pierwsza linia `setup()`), kilka ms później
  niż wcześniej; MAVLink TIMESYNC wchłania przesunięcie. TIM5 zatrzymuje
  się przy zatrzymaniu przez debugger.
- Poprawka w kroku 3 (wcielona w jego commit przed pushem): statystyki
  czasu wykonania FreeRTOS programowały TIM5 na 50 kHz w
  `vConfigureTimerForRunTimeStats`, co spowolniłoby `time_boot_us()`
  50 razy po starcie schedulera. Hooki są teraz w `time_base.cpp` i czytają
  `time_boot_us() >> 4` (16 us na jednostkę, przepełnienie po ~19 h zamiast
  ~23,8 h). Skutek uboczny widoczny w Monitorze: pierwszy task po starcie
  schedulera dostaje przypisany cały czas od `clock::init()` (kilka s), co
  zawyża jego licznik w pierwszym raporcie.
- Mapa timerów (obie płytki): TIM5 zegar; TIM10/11/13/14 PWM silników;
  enkodery rosbota TIM2/3/4/8; enkodery XL TIM1/2/3/4, wentylator TIM8.
- Po kroku 5 nasz kod nie wołał już żadnego API Arduino dla GPIO, czasu,
  ADC ani timerów. Na Arduino zostawały: `HardwareSerial` (link, debug,
  power board), `Wire`/`SPI`, Adafruit BNO055, Pololu VL53L0X,
  STM32Ethernet/LwIP, wejście `setup()`/`loop()` oraz inicjalizacja zegara
  i HAL przez core.
- Różnice zachowania z kroku 8: HAL_GetTick() na rosbot_xl to własny tick
  HAL (SysTick core zwiększa go przed i po starcie schedulera), a nie
  nadpisanie ze STM32Ethernet zwracające tick FreeRTOS; ta sama częstotliwość
  1 ms. Task ethernetu, jego priorytet i stos, sprawdzanie linku co 500 ms
  i opcje lwIP bez zmian.
- Krok 9 przeniósł z forka core Husarion: sekcję ETH z
  `hal_conf_custom.h` (MAC_ADDR*, ETH_RX/TX_BUF_SIZE/NB, przemapowane
  stałe `PHY_*` dla LAN9303, których używa sam `HAL_ETH_Init`) i
  sterownik HAL ETH sprzed 1.27 (`Legacy/stm32f4xx_hal_eth.c` z
  `HAL_ETH_LEGACY_MODULE_ENABLED`, kod identyczny z forkiem); źródła lwIP
  2.2.1 (STM32duino LwIP).
- Różnice zachowania z kroku 7: init BNO055 zapisuje remap osi i źródło
  zegara w jednym oknie CONFIG zamiast trzech dodatkowych przełączeń
  trybu (te same rejestry końcowe, krótszy start); `enableDmaReads()`
  zniknęło (DMA działa zaraz po `init()`); NACK na I2C kończy odczyt DMA
  od razu zamiast po 8 ms timeoutu; VL53L0X i EEPROM zachowują sekwencję
  Wire zapis-STOP-odczyt (`I2c::writeThenRead`). PI/constrain z Arduino
  zamienione na `kPi` / `std::clamp` (te same wartości; `kPi` zachowuje
  RAD_PER_TICK bit w bit).
- Różnice zachowania z kroku 6: bajty RX nie kosztują już przerwania każdy
  (921600 baud to do ~92 tys. przerwań/s); zapisy tekstu debug są
  kolejkowane i obsługiwane przez DMA w taskach, a przed schedulerem
  wysyłane pollingiem; `printf` obcina na 255 znakach (raport Monitora idzie
  przez `print`). BRR nadal liczony jak w HAL (USART3 +1,27 %); zwykłe
  zaokrąglenie dałoby -0,93 %, to osobna zmiana do sprawdzenia na HW, jeśli
  kiedyś będzie potrzebna. Dwa stare niedopasowania formatu `printf`
  (`%d` dla `BaseType_t`, `%u` dla `unsigned long`) wyszły dzięki
  atrybutowi `format` i są poprawione.
- Różnice zachowania z kroku 5: ADC1 jest konfigurowany raz zamiast pełnej
  inicjalizacji/deinicjalizacji HAL przy każdym `analogRead()` (odczyt
  spada z kilkudziesięciu us do ~1,2 us); odczyty są serializowane, a
  wcześniej taski silników i wentylatora na XL mogły przeplatać
  reinicjalizacje ADC1.

- Decyzje w kroku 9:
  - **Bez `.ioc` CubeMX.** STM32CubeIDE 2.0 nie ma już wbudowanego CubeMX,
    a samodzielnego CubeMX tu nie ma; poza tym każde peryferium jest już
    skonfigurowane w `lib/platform/`, więc wygenerowany kod inicjalizacji
    by to dublował. Drzewo zegarów jest kodem w
    `boards/stm32f407/system.cpp`. `.ioc` można dodać później jako
    dokumentację pinoutu.
  - **Sterta zostaje, alokacja statyczna odłożona (ustalenie 7 otwarte).**
    `pvPortMalloc()` to `malloc()` z newlib na całym wolnym RAM, tak jak w
    `heap_useNewlib_ST` ze STM32FreeRTOS, więc przełączenie nie zmienia
    budżetu RAM. Przejście na `xTaskCreateStatic` to osobna zmiana.
  - **Bez przerwania USART (ustalenie 1 niekontynuowane).** Task linku jest
    okresowy i czyta z timeoutem 0; wybudzanie przez IDLE opłaca się
    dopiero po przeprojektowaniu tego taska tak, żeby blokował się na RX, a
    to wymaga pomiarów czasów na HW.
  - **Projekt CMake w `cmake/`** z COLCON_IGNORE: `CMakeLists.txt` w roocie
    sprawiał, że colcon widział `rosbot_firmware (cmake)` i gubił
    `bridge/rosbot_mavlink_bridge` (sprawdzone przez `colcon list`).
  - `-Os` zachowane w obu presetach (ustalenie 5 zmierzone, bez zmiany).
- Różnice zachowania z kroku 9 do obserwacji na HW:
  - Kompilator: CI i `apt` dają GCC 13.2.1; PlatformIO używał 9.2.1. Oba
    budują bez ostrzeżeń (sprawdzone lokalnie także z 9.2.1 z PlatformIO).
  - Startup to szablon CMSIS + nasz konstruktor `boardInit`: to samo drzewo
    zegarów (wewnętrzny RC, 168 MHz), grupowanie NVIC 4, inicjalizacja HAL
    przed innymi konstruktorami, `VTOR` = flash. SystemInit nie resetuje
    już ponownie RCC (po resecie i tak ma wartości domyślne).
  - Wektory usterek wskazują bezpośrednio na `fatalError()`; kopia tablicy
    wektorów w RAM zniknęła. NMI też woła `fatalError()`.
  - Hooki idle i tick wyłączone (oba były puste).
  - `xPortGetFreeHeapSize()` (Monitor) = wolne bloki malloc + nigdy nie
    użyta sterta; STM32FreeRTOS raportował tylko nigdy nie użytą część,
    więc liczba może być wyższa.
  - Wektory przerwań USART1/2/3 to `Default_Handler`; nic ich nie włącza.
- Po review (po kroku 10): przed startem schedulera sterta nie rośnie poza
  bieżący SP, `malloc` z przerwania kończy się `configASSERT`; magistrala
  I2C jest odzyskiwana po błędzie (`I2c::recover()`); błąd transferu TX
  DMA w UART nie zatrzymuje już nadawania.
- Otwarte po kroku 10: `.vscode/launch.json` to wciąż konfiguracja
  debuggera PlatformIO (lokalny stan IDE, nieruszany); flashowanie przez
  ST-Link nie ma przepisu (flash idzie przez bootloader).

## Test HW dla kroków 1–10

Oba roboty, firmware z `just build rosbot[_xl][_release]` na testowanym
commicie:

- Start: czerwona dioda świeci, potem dioda statusu jak wcześniej (świeci,
  dopóki bridge się nie połączy, gaśnie, gdy wszystko OK).
- Gesty przy starcie: krótkie wciśnięcie → transport diagnostyczny
  (potwierdzenie zieloną diodą), przytrzymanie 3 s → okno kalibracji IMU
  (czerwona miga, zielona świeci po sukcesie).
- Jazda do przodu/tyłu/obrót i stop; hamulec trzyma; brak szarpnięć przy
  zmianie trybu. Częstotliwość `/joint_states` (klasa 100 Hz) bez zmian.
- ROSbot: 4 czujniki odległości publikują (sekwencja XSHUT), obie diody
  panelu reagują na `ROSBOT_PANEL_LEDS`, oba przyciski w `ROSBOT_BUTTONS`.
- ROSbot XL: ścieżka limitu prądu (ILIM) rev 1.1 / 1.2, przycisk wyłączenia
  na power boardzie → żądanie wyłączenia + linia potwierdzenia, hub USB
  działa, wzmacniacz audio włączony.
- PWM / ADC (kroki 4–5): wszystkie cztery koła płynnie śledzą zadaną
  prędkość; napięcie `/battery` na ROSbocie zgadza się z multimetrem jak
  wcześniej; na XL rev 1.2 effort w `_motors/feedback` rośnie z obciążeniem
  (pomiar prądu CSO), wentylator XL przyspiesza z temperaturą, a
  temperatura z NTC jest wiarygodna.
- UART (krok 6): bridge ROSbota łączy się przez link SBC i trzyma
  połączenie 10+ min przy pełnej telemetrii (bez błędów CRC MAVLink w logu
  bridge'a); handshake przy starcie (FW/NS/BACKEND/END) działa; tylne FTDI
  pokazuje tekst startowy i raport Monitora (build debug); FTDI jako link
  po krótkim wciśnięciu przycisku; tekst kalibracji IMU przy starcie
  zarówno na FTDI, jak i na linku SBC. XL: tekst debug na USART1,
  `/battery` z power boardu (USART2), serwis wersji PB.
- I2C/SPI (krok 7): osie orientacji IMU bez zmian na obu robotach (`/imu`
  z robotem poziomo, przechylonym, obróconym); `/imu` w zwykłej
  częstotliwości bez timeoutów; okno kalibracji IMU przy starcie i
  zapisane offsety zastosowane przy następnym starcie; zapis kalibracji w
  runtime; 4 czujniki odległości ROSbota z poprawnym frame dla każdego;
  rewizja płytki XL odczytana z EEPROM (odpowiednia ścieżka ILIM /
  wentylatora); animacje taśmy LED na XL.
- Ethernet (krok 8, XL): bridge łączy się przez UDP przy starcie i po
  ponownym wpięciu kabla (sprawdzanie linku); ping 192.168.77.3 z SBC;
  przycisk wyłączenia na power boardzie wysyła żądanie HTTP do SBC; porty
  LAN9303 nadal przekazują ruch (SBC <-> tylny port).
- Przełączenie (krok 9): start na obu płytkach z buildami debug i
  release; kalibracja IMU i namespace zapisane przed przełączeniem nadal
  się odczytują (sektor 11 nietknięty); wymuszona usterka (np.
  `configASSERT(0)` z debuggera) resetuje i zgłasza „MCU reset after a
  firmware fault”; Monitor (build debug) pokazuje zapas stosów i idle %
  bliskie wartościom z kroku 8; `just flash` działa na SBC.
- Odzyskiwanie I2C (po kroku 10): przy działającym `/imu` zewrzeć SDA
  magistrali IMU do masy na ~1 s (ROSbot: I2C3 PC9, XL: I2C2 PF0) i
  puścić; `/imu` wraca po kilku odczytach bez restartu, a na XL EEPROM
  nadal odpowiada (po restarcie właściwa rewizja płytki). Opcjonalnie to
  samo z SCL.
- Czasy (krok 3): TIMESYNC bridge'a zbiega się i trzyma (bez skoków
  zegara w znacznikach `/imu` / `/joint_states` przez 10+ min); watchdog
  kół zatrzymuje robota ~0,5 s po ustaniu komend; kalibracja IMU w runtime
  (komenda `ROSBOT_IMU_CALIBRATION`) nadal zapisuje offsety; idle % taska
  Monitor względem bazowych wartości z 2026-10 (build debug).

## Wyniki testów HW: ROSbot 3, jazzy vs nowy firmware (2026-11-19)

Ta sama płytka CORE2, bateria 12,3 V, koła w powietrzu. Link MAVLink przez
tylny USB (FTDI) dzięki testowej fladze `FORCE_DIAGNOSTIC_LINK`, ta sama w
obu wersjach; obciążenie CPU, zapas stosów i opóźnienie „próbka → kolejka
TX” z identycznej instrumentacji w obu kopiach. Mediany z 6 przebiegów
jazzy i 3–10 przebiegów nowego (`-O2`, link 1 kHz). Szczegóły, surowe dane i skrypty:
`~/Husarion/reports/rosbot/cube-fw-metrics-2026-11-19.html` (+ katalog obok; nie w repo).

| Metryka | jazzy | nowy | Lepiej |
|---|---:|---:|---|
| CPU idle, bez jazdy | 92,8 % | 96,1 % | nowy |
| CPU idle, jazda kół | 92,6 % | 95,9 % | nowy |
| Task Imu | 3,75 % | 0,43 % | nowy |
| Task Range | 1,26 % | 0,32 % | nowy |
| Task MotorControl | 0,80 % | 0,64 % | nowy |
| Task Link | 1,30 % | 2,38 % | jazzy (cena 1 kHz) |
| Próbka IMU → TX, średnio / max | 5,95 / 10,05 ms | 3,53 / 6,98 ms | nowy |
| Feedback kół → TX, średnio / max | 88 / 198 µs | 65 / 114 µs | nowy |
| Odchylenie odstępów IMU / feedback | 1,89 / 1,11 ms | 1,68 / 0,60 ms | nowy |
| Max odstęp na hoście IMU / feedback | 17,5 / 15,1 ms | 29,2 / 27,3 ms | jazzy (patrz niżej) |
| Częstotliwości IMU / feedback / ranges | 99,8 / 199,6 / 40,0 Hz | bez zmian | — |
| Start do pierwszego MAVLink | 5,64 s | 5,29 s | nowy |
| Odpowiedź serwisu `_mcu_id` | 5,0 ms | 3,0 ms | nowy |
| Wolna sterta | 89,4 KB | 99,4 KB (102,5 KB bez Monitora) | nowy |
| Zapas stosu Imu / Range / Link | 604 / 396 / 2760 B | 468 / 372 / 2584 B | jazzy (wciąż duży zapas) |
| Zapas stosu Battery / MotorControl | 272 / 608 B | 308 / 608 B | nowy / = |
| Obraz (flash) | 88 KB | 100 KB | jazzy (11 % z 896 KB) |
| Statyczny RAM | 11,4 KB | 9,3 KB | nowy |
| Śledzenie prędkości kół, watchdog | 2,95 rad/s, 548 ms | 2,93–2,98 rad/s, 546–551 ms | = |
| Szum żyroskopu w spoczynku | 0,0013 rad/s | 0,0013 rad/s | = |
| Diagnostyka w release | brak | `/diagnostics` 1 Hz | nowy |

Max odstęp na hoście: nowy firmware wysyła próbkę zaraz po odczycie, więc
zmienny czas odczytu BNO055 (clock stretching, 1,5–7 ms) widać w czasie
dostarczenia; jazzy publikował na sztywnej siatce 5 ms, która to maskowała
kosztem większego opóźnienia. Znaczniki czasu próbek są regularne w obu.

Usterki sterownika silników (nFAULT) przy kołach w powietrzu występują w
obu wersjach w podobnej liczbie (0–12 na przebieg); zaniżają pojedyncze
przebiegi prędkości ustalonej, dlatego mediany.

Błędy znalezione na HW i poprawione w tej serii:
- `Pin::setAsOutput(true)` przed włączeniem zegara portu (krok 2): pin
  adresu BNO055 startował nisko, IMU nieznalezione.
- Wyścig HEARTBEAT w `MavlinkNode::loop()` (był też w jazzy): zrywanie
  linku co kilka sekund przy dłuższym `drainRx()`.
- Niezasilony/zawieszony BNO055 wywoływał burzę przerwań I2C i zawieszał
  MCU (link milknie, bez resetu); jazzy w tej sytuacji działał.
- Licznik błędów RX MAVLink zawsze 0 (biblioteka nie wypełnia
  `packet_rx_drop_count` kanału).
- Mój błąd z kroku 7: zapis UNIT_SEL = rad/s (żyroskop ~1,8 % za nisko).

Wypróbowane i odrzucone: budzenie taska Link przez producentów próbek. W
4 z 4 sesji przedni VL53L0X przestawał odpowiadać (NACK pod swoim adresem)
po testach kół; przyczyna niezrozumiana (ARCHITECTURE.md, I2C).

Nie testowane zdalnie (wymaga człowieka przy robocie albo SBC): gesty
przycisku przy starcie, diody, przyciski, osie IMU przy przechylaniu,
okno kalibracji IMU, link przez UART SBC, fizyczne zwarcie SDA (zastąpione
programowym).

Regresja po poprawce I2C (2026-11-20, HEAD 3ba02e8): IMU, feedback kół,
silniki (2,98 rad/s), `/ranges` 40 Hz, 0 timeoutów czujników. Przebiegi,
w których koła ocierały o ścianę, odrzucone (MCU miał wtedy reset od
spadku napięcia, potem wszystkie VL53L0X miały timeouty aż do resetu,
ARCHITECTURE.md „Range sensors that lose power”).

## Wyniki testów HW: ROSbot XL, jazzy vs nowy firmware (2026-11-20)

Digital Board v1.2 (rewizja z EEPROM: V1_2), power board v1.3 z firmware
v1.0.4, koła w powietrzu, link UDP przez Ethernet do hosta (192.168.77.2).
Flashowanie i pomiary przez ST-LINK (SWD): ten sam task statystyk w obu
wersjach zapisuje obciążenie CPU, zapas stosów, stertę i opóźnienie
„próbka → kolejka TX” do RAM, a host czyta je przez SWD (log z innego
taska psuł lwIP jazzy). Mediany z 5 przebiegów każdej wersji, naprzemiennie,
świeży flash przed każdym.

| Metryka | jazzy | nowy | Lepiej |
|---|---:|---:|---|
| CPU idle bez jazdy / w jeździe (statystyki FreeRTOS) | 91,8 / 88,5 % | 95,0 / 94,7 % | nowy |
| CPU idle z próbkowania PC przez SWD (stan ustalony) | 90,7 % | 94,0 % | nowy |
| Task Imu | 3,72 % | 0,45 % | nowy |
| Task MotorControl bez jazdy / w jeździe | 0,80 / 3,92 % | 0,64 / 1,03 % | nowy |
| Task Link | 3,25 % | 2,72 % | nowy |
| Task ethernet_scheduler | 0,04 % | 0,81 % | jazzy (lwIP co tick) |
| Próbka IMU → TX, średnio / max | 3,67 / 7,94 ms | 3,65 / 7,96 ms | = |
| Feedback kół → TX bez jazdy / w jeździe | 95 / 265 µs | 78 / 96 µs | nowy |
| Częstotliwości IMU / feedback | 99,7 / 199,4 Hz | 99,7 / 199,5 Hz | = |
| `/battery` | 0,91 Hz | 9,97 Hz | nowy (czyta nowy firmware PB) |
| Wolna sterta (z taskiem statystyk) | 44,1 KB | 55,3 KB | nowy |
| Statyczny RAM (data + bss) | 56,0 KB | 52,6 KB | nowy |
| Obraz (flash) | 122,7 KB | 152,0 KB | jazzy |
| Najmniejszy zapas stosu | HwMonitor 268 B | HwMonitor 156–252 B → po `3ba02e8` IDLE 432 B | nowy po poprawce |
| Od resetu do połączenia z bridge'em | 11,3 s | 11,3 s | = |
| Śledzenie prędkości kół / start do 90 % / watchdog | 3,00 rad/s / 36 ms / 561 ms | 3,00 rad/s / 35 ms / 559 ms | = |
| Odpowiedź `_mcu_id` | 1,7 ms | 1,7 ms | = |
| Szum żyroskopu (std) | 0,001 rad/s | 0,001 rad/s | = |
| 10 min ciągłej pracy | — | 0 zerwań, 0 cofnięć znaczników | — |
| Korekty TIMESYNC (dryf HSI) | co ~30 s | co ~35–40 s | = (oba na HSI) |

Próbkowanie PC przez SWD potwierdza statystyki FreeRTOS: w stanie
ustalonym różnią się o 0,4–1,3 punktu (czas przerwań). Pierwsza seria
próbkowała też start MCU (opóźnienia, autonegocjacja Ethernetu) i dawała
zaniżone idle; powtórzone na ustalonej pracy.

Start XL (próbkowanie PC od resetu, oba firmware podobnie): ~1,6 s
opóźnień przy starcie, ~2,5 s okna konfiguracji (`waitForHostConfig`),
~5 s czekania HAL na autonegocjację Ethernetu, potem scheduler.

Testy funkcjonalne XL na buildzie release z HEAD: ping 192.168.77.3,
`_mcu_id`, `_power_board_info` (v1.0.4 / v1.3), zielona dioda z `/leds`
(GPIOE ODR), ramka taśmy LED w buforze `g_led_strip`, effort z pomiaru
prądu ~0,09 Nm przy 3 rad/s, NTC 27,4 °C, PWM wentylatora TIM8 CH2N 25 kHz
(wymuszone 50 % → ~54 % próbek wysokich na PB0), wzmacniacz audio
włączony (PB2), hub USB poza resetem (PE1), tekst debug na USART1 (drugie
FTDI), sesja kalibracji IMU start/stop, zapisane offsety nadal stosowane
po flashowaniu (sektor 11 nietknięty), śmieci na RX → Link WARN,
wstrzyknięte błędy przez debugger: asercja, hard fault (PC 0x40000000),
przepełnienie stosu → właściwa przyczyna w `/diagnostics` po restarcie;
zwarcie SDA (PF0 wymuszony nisko przez SWD na 1 s) → jeden reset
magistrali, IMU wraca bez restartu, EEPROM nadal czytany (rewizja V1_2 po
kolejnych resetach). Przy podłączonym debuggerze `fatalErrorWithCode`
zatrzymuje się na `BKPT` (tak jest zaprojektowane), test go przeskakuje.

Błędy znalezione na XL i poprawione:
- IMU nie działało wcale (`205e691`): stary callback DMA po timeoucie
  przy starcie włączał `DMAEN` przy zapisie IT i każdy odczyt kończył się
  burzą przerwań (ARCHITECTURE.md, I2C fault recovery). Ten sam błąd był
  uśpiony na ROSbocie.
- Zapas stosu HwMonitor 156–252 B, blisko progu ostrzeżenia (`3ba02e8`).

Rozwiązane później tego samego dnia (2026-11-20):
- Timeouty IMU przy starcie: BNO055 rozciąga zegar >8 ms przez ~120 ms po
  przejściu w fuzję; timeout odczytu 25 ms → 0 błędów (`4516a34`).
- Zegar z kwarcu (`7a1d068`, spec `.claude/specs/hse-clock.md`): XL −9 ppm,
  0 korekt TIMESYNC w 10 min (było 17), ROSbot IMU 100,0 / feedback
  200,0 Hz; fallback na HSI sprawdzony.
- Martwe VL53L0X (`1dd2da8`): wykrycie w ~200 ms bez kręcenia CPU, ponowna
  inicjalizacja całej tablicy 1→16 s, sprawdzone impulsem XSHUT na HW
  (idle 95,3 % przez cały czas); w teście kół przedni prawy czujnik raz
  się zgubił i sam wrócił.

Wycieki pamięci (2026-11-20): 30 min obciążenia (komendy kół, taśma LED,
diody, serwisy, śmieci UDP, restart bridge'a co 5 min) na obu robotach.
ROSbot: wolna sterta stała 102 504 B. XL przez SWD co minutę: sterta newlib
20 884 B zajęte, koniec `sbrk` stały, sterta lwIP pusta, pule `memp` bez
zmian (UDP PCB 1/6 zajęty przez 6 połączeń). Brak wycieku. Przegląd kodu:
wszystkie alokacje przy starcie, ścieżki ponownego otwarcia sprawdzają
`nullptr`, pbufy zwalniane na każdej ścieżce.

Szarpnięcia silników XL przy zmianie kierunku (2026-11-20): jazzy i nowy
firmware identyczne. Skok +2 → −2 rad/s prosto na `/_motors/cmd`: pełne
PWM wstecz, odwrócenie w ~25 ms, przestrzelenie do −2,97, powrót do −1,47,
ustalenie ~110 ms (szczyt ~150–175 rad/s²). Z rampą jak z kontrolera
(1 m/s² ≈ 20 rad/s² na kole) płynnie, ~10–15 ms przy zerze. Pod obciążeniem
możliwa „blokada” przy zerze: zadana 0 kasuje całkę, a wypełnienie <1 %
włącza Brake (zwarcie). Poprawka regulatora czeka na decyzję.

Testy przy robocie XL (2026-11-20, HEAD z kwarcem, Rafał przy robocie):
- przycisk 1 w `/buttons` (6 z 6 wciśnięć), zielona dioda z `/leds`, taśma
  LED czerwony/zielony/niebieski;
- osie IMU zgodne z REP-103: lewy bok w dół → roll −37°, przód w dół →
  pitch +29°, obrót w lewo → yaw +72°, gyro z tymi samymi znakami;
- gest krótki przy starcie: zielona od razu na stałe, powitanie `FW:` na
  tylnym FTDI, MAVLink po UDP wraca ~10 s po resecie;
- gest długi: okno kalibracji IMU. Błąd z jazzy: zapisane offsety
  wgrywane przed oknem dawały od razu „fully calibrated” i zapisywały je
  ponownie, ponowna kalibracja była niemożliwa. Poprawione w `934a2f2`
  (offsety po oknie, tylko gdy się nie udało); na HW czerwona miga, okno
  startuje od accel 0 / mag 0, timeout po 120 s, stare offsety wracają
  (`has_saved` = 1). Na ROSbocie ten sam kod, bez testu na HW;
- wypięcie kabla Ethernet na ~10 s: bridge zgłasza rozłączenie po 3 s,
  link wraca sam po wpięciu, bez resetu MCU;
- wentylator: NTC 33,1 °C → 29 %, 36,8 °C → 43 %, 29,6 °C → stop, zgodnie
  ze wzorem 30 °C/20 % … 55 °C/100 %;
- przycisk wyłączenia na power boardzie: `GET /shutdown` do 192.168.77.2:3000
  od razu, power board wyłącza robota ~10 s później. Uwaga: snap rosbot na
  laptopie deweloperskim (`rosbot.db-server`) po tym żądaniu wyłącza
  laptop; na czas testu `sudo snap stop rosbot.db-server`.

Testy przy robocie ROSbot 3 (2026-11-20, HEAD, ST-LINK już działa na
CORE2: wtyczka była wcześniej odwrócona):
- dwie zielone diody panelu z `/leds` (bity 0 i 1) ✔; przyciski: wszystkie
  4 wciśnięcia, ale przycisk 1 → bit 1 (`0x02`), przycisk 2 → bit 0 —
  tablica `{PUSH_BUTTON2, PUSH_BUTTON1}` jak w jazzy, mapowanie nigdzie
  nieopisane (do dopisania w ROS_API.md / kontrakcie z rosbot_ros);
- osie IMU zgodne z REP-103 (roll −28°, pitch +31°, yaw +70°);
- krótki gest: zielone od razu na stałe ✔. MAVLink zostaje na UART SBC
  (jak w jazzy), więc bez SBC test linku przez tylne USB tylko z flagą
  `FORCE_DIAGNOSTIC_LINK`;
- długi gest z poprawką `934a2f2`: czerwona miga, po oknie zapisane
  offsety wracają (`has_saved` = 1, sys/gyro/accel/mag = 3);
- 4 czujniki odległości reagują na dłoń, właściwe `frame_id`;
- fizyczne zwarcie SDA pominięte (programowy odpowiednik przeszedł).
- Odwrócona wtyczka SWD: CORE2 sprawny (linie MCU przez 330 Ω); NRST
  ST-LINK-a nie resetuje teraz CORE2 (impuls 100 ms bez efektu, uptime
  rośnie) — możliwe uszkodzenie wyjścia NRST w ST-LINK-u (~150 mA przy
  próbie resetu z NRST na odwróconej wtyczce) albo brak przewodu w
  adapterze; SWD i reset programowy działają.

Nie testowane na XL: porty LAN9303 (SBC ↔ tylny port) i `just flash` z SBC
(brak SBC).

## Strategia testów

- Każdy krok: build wszystkich wariantów (teraz `just build-all`), testy
  hosta (wcześniej `pio test -e native`, teraz `just test`), pre-commit.
- Czyste refaktory: porównanie binarek release (albo znormalizowanego
  disassembly) z commitem nadrzędnym.
- Kroki HW: smoke test na obu robotach: handshake z bridge'em przy
  starcie, częstotliwości `/joint_states` + `/imu` (ARCHITECTURE, „Timing
  measured on HW”), reakcja na komendy kół, bateria, diody, kalibracja IMU,
  gest zmiany transportu; liczby stosów i CPU z taska Monitor względem
  bazowych z 2026-10.
- Test zdalny przez tylny USB (bez SBC i bez przycisku): tymczasowa flaga
  kompilacji `FORCE_DIAGNOSTIC_LINK` (tylko w kopii źródeł do testu, nie w
  repo) wymusza link MAVLink na FTDI, tak samo w buildzie jazzy i nowym.
