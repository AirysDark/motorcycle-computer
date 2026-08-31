#pragma once
#include "bike/lighting.hpp"

namespace bike {

class LightingHardware {
public:
    virtual ~LightingHardware() = default;

    virtual bool write_output(LightingOutput output, bool enabled) = 0;
    virtual bool read_output(LightingOutput output) const = 0;
};

} // namespace bike
