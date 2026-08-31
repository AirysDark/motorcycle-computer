#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include "bike/transport.hpp"

class PosixSerialTransport final : public bike::Transport {
public:
    PosixSerialTransport() = default;
    ~PosixSerialTransport() override;

    PosixSerialTransport(const PosixSerialTransport&) = delete;
    PosixSerialTransport& operator=(const PosixSerialTransport&) = delete;

    bool open_device(const std::string& path, std::uint32_t baud = 115200);
    void close_device();
    bool is_open() const { return fd_ >= 0; }

    bool write(const std::uint8_t* data, std::size_t length) override;
    std::size_t read(std::uint8_t* data, std::size_t capacity) override;

private:
    int fd_{-1};
};
