#pragma once

#include <SPI.h>
#include "transport.hpp"

struct SpiTransportConfig {
    uint32_t mosi_pin;
    uint32_t miso_pin;
    uint32_t sck_pin;
    uint32_t spi_speed;
    BitOrder bit_order;
    uint8_t  spi_mode;
};

/// SPI transport implementation using STM32 HW SPI.
class SpiTransport : public Transport {
public:
    explicit SpiTransport(const SpiTransportConfig& cfg);

    bool init() override;
    void transfer(uint8_t byte) override;

private:
    SpiTransportConfig cfg_;
    SPIClass spi_;
};