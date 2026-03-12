#pragma once

#include <Arduino.h>
#include <math.h>

// ─── Configuration ───────────────────────────────────────────────
struct NtcConfig {
    uint8_t  pin;                  // analog input pin
    float    pullup_resistance;    // pull-up resistor value [Ohm]
    float    c1;                   // Steinhart-Hart coefficient
    float    c2;                   // Steinhart-Hart coefficient
    float    c3;                   // Steinhart-Hart coefficient
    float    offset  = 273.15f;    // Kelvin -> Celsius
    float    adc_max = 1023.0f;    // 10-bit: 1023, 12-bit: 4095
};

// ─── NTC Temperature Sensor ─────────────────────────────────────
class Ntc {
 public:
    explicit Ntc(const NtcConfig& config);

    void init();

    float   readCelsius()  const;

 private:
    float   adcToResistance(float adc_value) const;
    float   resistanceToCelsius(float resistance) const;

    NtcConfig config_;
};