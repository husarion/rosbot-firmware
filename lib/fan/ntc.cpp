#include "ntc.hpp"

// ─── Constructor ─────────────────────────────────────────────────
Ntc::Ntc(const NtcConfig& config)
    : config_(config)
{}

// ─── Init ────────────────────────────────────────────────────────
void Ntc::init() {
    pinMode(config_.pin, INPUT);
}

// ─── Public: read temperature ────────────────────────────────────
float Ntc::readCelsius() const {
    const float adc_value = static_cast<float>(analogRead(config_.pin));

    // Guard: ADC saturated -> resistance would be infinite
    if (adc_value >= config_.adc_max) {
        return -127.0f;
    }

    const float resistance = adcToResistance(adc_value);
    return resistanceToCelsius(resistance);
}

// ─── Private: ADC -> Resistance ─────────────────────────────────
//
//  Voltage divider: Vcc -> R_pullup -> ADC_pin -> NTC -> GND
//  R_ntc = R_pullup * ADC / (ADC_max - ADC)
//
float Ntc::adcToResistance(float adc_value) const {
    return config_.pullup_resistance * adc_value
           / (config_.adc_max - adc_value);
}

// ─── Private: Resistance -> Celsius (Steinhart-Hart) ────────────
//
//  1/T = c1 + c2*ln(R) + c3*ln(R)^3
//
float Ntc::resistanceToCelsius(float resistance) const {
    const float log_r = logf(resistance);
    const float inv_t = config_.c1
                      + config_.c2 * log_r
                      + config_.c3 * log_r * log_r * log_r;
    return (1.0f / inv_t) - config_.offset;
}