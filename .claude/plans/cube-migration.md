# Plan: Arduino → STM32Cube HAL + FreeRTOS + ETL

Status: kroki 0–7 na lokalnym `devel`, kroki 8–10 na lokalnym
`cube-switchover`, 2026-11-19. Test HW na ROSbocie 3 (CORE2, tylny USB)
trwa; ROSbot XL nietestowany.

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
