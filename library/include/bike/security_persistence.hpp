#pragma once

namespace bike {

class SecurityPersistence {
public:
    virtual ~SecurityPersistence() = default;

    // Returns true when a stored value was read successfully.
    virtual bool load_armed(bool& armed) = 0;
    virtual bool save_armed(bool armed) = 0;
};

} // namespace bike
