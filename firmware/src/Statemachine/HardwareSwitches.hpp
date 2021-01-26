#pragma once
#include <FreeRTOS.h>

namespace remote_control_device
{
class HardwareSwitchesState;

class HardwareSwitches
{
public:
    HardwareSwitches() = default;
    virtual ~HardwareSwitches() = default;

    HardwareSwitches(const HardwareSwitches&) = default;
    HardwareSwitches(HardwareSwitches&&) = default;
    HardwareSwitches& operator=(const HardwareSwitches&) = default;
    HardwareSwitches& operator=(HardwareSwitches&&) = default;

    virtual void update(HardwareSwitchesState &state) const;
};

} // namespace remote_control_device