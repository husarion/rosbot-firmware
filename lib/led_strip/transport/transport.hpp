#pragma once

#include <cstdint>

class Transport {
public:
    virtual ~Transport() = default;

    /// Initialize the transport (SPI peripheral, GPIO, etc.)
    /// @return true on success
    virtual bool init() = 0;

    /// Send a single byte over the transport.
    virtual void transfer(uint8_t byte) = 0;
};