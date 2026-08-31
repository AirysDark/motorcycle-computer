#pragma once
#include <cstddef>
#include <cstdint>

namespace bike {

class Transport {
public:
    virtual ~Transport() = default;

    virtual bool write(const std::uint8_t* data, std::size_t length) = 0;
    virtual std::size_t read(std::uint8_t* data, std::size_t capacity) = 0;
};

} // namespace bike
