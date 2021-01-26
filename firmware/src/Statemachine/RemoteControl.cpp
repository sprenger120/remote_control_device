#include "RemoteControl.hpp"
#include "Logging.hpp"
#include "PeripheralDrivers/ReceiverModule.hpp"
#include "StateSources.hpp"
#include <stm32f3xx_hal.h>

namespace remote_control_device
{

RemoteControl::RemoteControl(wrapper::HAL &hal, Logging &log, ReceiverModule &recv)
    : _hal(hal), _log(log), _receiverModule(recv)
{
}

void RemoteControl::update(RemoteControlState &target) const
{
    Frame frame;
    if (!_receiverModule.getSBUSFrame(frame))
    {
        _log.logWarning(Logging::Origin::StateMachine,
                   "Unable to aquire mutex in RemoteControl::update");
        target.timeout = true;
        return;
    }
    _update(frame, target);
}

void RemoteControl::_update(const Frame &frame, RemoteControlState &target) const
{
    static_assert(Deadband > Decoder::ANALOG_CHANNEL_MIN && Deadband < Decoder::ANALOG_CHANNEL_MAX);
    static_assert(SwitchOn_MinValue > Decoder::ANALOG_CHANNEL_MIN &&
                  SwitchOn_MinValue < Decoder::ANALOG_CHANNEL_MAX);

    static_assert(ChannelMap::Throttle < Protocol::ANALOG_CHANNEL_COUNT);
    static_assert(ChannelMap::Brake < Protocol::ANALOG_CHANNEL_COUNT);
    static_assert(ChannelMap::Steering < Protocol::ANALOG_CHANNEL_COUNT);
    static_assert(ChannelMap::ButtonEmcy < Protocol::ANALOG_CHANNEL_COUNT);
    static_assert(ChannelMap::SwitchAutonomous < Protocol::ANALOG_CHANNEL_COUNT);
    static_assert(ChannelMap::SwitchRemote < Protocol::ANALOG_CHANNEL_COUNT);
    static_assert(ChannelMap::SwitchUnlock < Protocol::ANALOG_CHANNEL_COUNT);

    target.throttle = channelToFloat(frame.analogChannels[ChannelMap::Throttle], true);
    target.brake = channelToFloat(frame.analogChannels[ChannelMap::Brake]);
    target.steering = channelToFloat(frame.analogChannels[ChannelMap::Steering], true);

    target.switchUnlock = channelToBool(frame.analogChannels[ChannelMap::SwitchUnlock]);
    target.switchRemoteControl = channelToBool(frame.analogChannels[ChannelMap::SwitchRemote]);
    target.switchAutonomous = channelToBool(frame.analogChannels[ChannelMap::SwitchAutonomous]);
    target.buttonEmergency = channelToBool(frame.analogChannels[ChannelMap::ButtonEmcy]);

    // throttle out of deadband = throttle is up
    target.throttleIsUp = std::abs(target.throttle) > 0.01;

    target.timeout = (_hal.GetTick() - frame.lastUpdate) > Timeout_Ms || frame.failsafe;
}

float RemoteControl::channelToFloat(const uint16_t value, const bool bidirectional) const
{
    static constexpr uint16_t AnalogMiddle = Decoder::ANALOG_CHANNEL_MAX / 2;
    float converted = static_cast<float>(value) / static_cast<float>(Decoder::ANALOG_CHANNEL_MAX);
    if (bidirectional)
    {
        if ((value < AnalogMiddle + Deadband) && (value > AnalogMiddle - Deadband))
        {
            return 0.0f;
        }
        return (converted * 2.0f) - 1.0f;
    }

    if (value < Deadband)
    {
        return 0.0f;
    }
    return converted;
}

bool RemoteControl::channelToBool(const uint16_t value) const
{
    return value > SwitchOn_MinValue;
}

} // namespace remote_control_device
