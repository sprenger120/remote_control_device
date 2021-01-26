#pragma once
#include <FreeRTOS.h>

namespace remote_control_device
{

struct BusDevicesState
{
    bool timeout = true;
    bool rtdEmergency = false;
    bool rtdBootedUp = false;
};

/**
 * @brief Viable states for a coupling to be in
 *
 */
enum class CouplingState : uint8_t
{
    Engaged,
    Disengaged
};

/**
 * @brief Viable states for a can device to have
 *
 */
enum class CanDeviceState : uint8_t
{
    Preoperational,
    Operational,
    Unknown
};
const char *getCanDeviceStateName(const CanDeviceState state);

struct RemoteControlState
{
    // -1 to 1
    float throttle = 0.0;
    // 0 to 1
    float brake = 0.0;
    // -1 to 1
    float steering = 0.0;

    bool switchUnlock = false;
    bool switchRemoteControl = false;
    bool switchAutonomous = false;
    bool buttonEmergency = false;

    bool throttleIsUp = false;
    bool timeout = true;
};

/**
 * @brief Reads out hardware switches connected to the board
 *
 */
struct HardwareSwitchesState
{
    bool ManualSwitch = false;
    bool BikeEmergency = false;
};

} // namespace remote_control_device