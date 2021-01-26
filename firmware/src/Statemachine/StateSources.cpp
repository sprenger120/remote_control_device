#include "StateSources.hpp"

namespace remote_control_device
{

const char *getCanDeviceStateName(const CanDeviceState state)
{
    switch (state)
    {
        case CanDeviceState::Preoperational:
            return "Preoperational";
        case CanDeviceState::Operational:
            return "Operational";
        case CanDeviceState::Unknown:
            return "Unknown";
    }
    return "Name not defined for state";
}

} // namespace remote_control_device