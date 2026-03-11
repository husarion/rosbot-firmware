#include "spi_transport.hpp"
#include <STM32FreeRTOS.h>

SpiTransport::SpiTransport(const SpiTransportConfig& cfg)
    : cfg_(cfg)
    , spi_(cfg.mosi_pin, cfg.miso_pin, cfg.sck_pin)
{}

bool SpiTransport::init() {
    SPISettings settings(
        cfg_.spi_speed,
        cfg_.bit_order,
        cfg_.spi_mode,
        SPI_TRANSMITONLY
    );
    spi_.beginTransaction(CS_PIN_CONTROLLED_BY_USER, settings);
    return true;
}

void SpiTransport::transfer(uint8_t byte) {
    spi_.transfer(CS_PIN_CONTROLLED_BY_USER, byte);
}
