#pragma once
#include "SBUSDecoder.hpp"
#include "Wrapper/HAL.hpp"
#include <FreeRTOS.h>

/**
 * @brief Handles Decoded S.BUS Frames and converts it to RemoteControlState
 *
 */

namespace remote_control_device
{
using namespace SBUS; // NOLINT

class RemoteControlState;
class Logging;
class ReceiverModule;

class RemoteControl
{
public:
    explicit RemoteControl(wrapper::HAL &hal, Logging &log, ReceiverModule& recv);
    virtual ~RemoteControl() = default;

    RemoteControl(const RemoteControl &) = default;
    RemoteControl(RemoteControl &&) = default;
    RemoteControl &operator=(const RemoteControl &) = default;
    RemoteControl &operator=(RemoteControl &&) = default;

    /**
     * @brief Blocking. Retrieves the newest S.BUS frame from ReceiverModule driver and converts it
     *
     * @param target target to write data to
     */
    virtual void update(RemoteControlState &target) const;
    virtual void _update(const Frame &frame, RemoteControlState &target) const;

    struct ChannelMap
    {
        static constexpr uint8_t Throttle = 2;
        static constexpr uint8_t Brake = 0;
        static constexpr uint8_t Steering = 1;
        static constexpr uint8_t ButtonEmcy = 4;
        static constexpr uint8_t SwitchAutonomous = 6;
        static constexpr uint8_t SwitchRemote = 7;
        static constexpr uint8_t SwitchUnlock = 8;
    };

    static constexpr uint16_t Deadband = 10;
    static constexpr uint16_t SwitchOn_MinValue = 900;
    static constexpr uint16_t Timeout_Ms = 500;

private:
    wrapper::HAL &_hal;
    Logging &_log;
    ReceiverModule &_receiverModule;

    float channelToFloat(const uint16_t, const bool bidirectional = false) const;
    bool channelToBool(const uint16_t) const;
};

} // namespace remote_control_device