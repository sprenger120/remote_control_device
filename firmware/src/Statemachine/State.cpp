#include "State.hpp"

namespace remote_control_device
{
const char *getStateIdName(const StateId id)
{
    switch (id)
    {
        case StateId::Start:
            return "Start";
        case StateId::Idle:
            return "Idle";
        case StateId::RemoteControl:
            return "RemoteControl";
        case StateId::Autonomous:
            return "Autonomous";
        case StateId::SoftEmergency:
            return "SoftEmergency";
        case StateId::Manual:
            return "Manual";
        case StateId::Emergency:
            return "Emergency";
        case StateId::NO_STATE:
            return "NO_STATE";
    }
    return "Unknown State";
}

} // namespace remote_control_device