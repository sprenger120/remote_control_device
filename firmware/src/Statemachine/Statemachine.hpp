#pragma once
#include "SpanCompatibility.hpp"
#include "StateSources.hpp"
#include "States.hpp"
#include "Wrapper/HAL.hpp"
#include "Wrapper/Task.hpp"
#include <FreeRTOS.h>
#include <array>
#include <stm32f3xx_hal_iwdg.h>

namespace remote_control_device
{
class Canopen;
class LEDUpdater;
class RemoteControl;
class HardwareSwitches;
class LEDUpdater;
class TerminalIO;
class CanFestivalTimers;

/**
 * @brief Sources that are processed to decide the device's state
 *
 */
struct StateChaningSources
{
    BusDevicesState const &busDevicesState;        // NOLINT
    RemoteControlState const &remoteControl;       // NOLINT
    HardwareSwitchesState const &hardwareSwitches; // NOLINT
    StateId const &currentState;                   // NOLINT
    Canopen &canopen;                              // NOLINT

    StateChaningSources(const StateId &state, Canopen &co, bool &startedUp,
                        const BusDevicesState &busDevicesState,
                        const RemoteControlState &remoteControl,
                        const HardwareSwitchesState &hardwareSwitches);
    void sigalStartedUp();
    bool isStartedUp() const;

private:
    bool &_startedUp;
};

/**
 * @brief Controls the "high level" behaviour of this device. See Statemachine.cpp for states
 * implementations.
 *
 */
class Statemachine
{
public:
    static constexpr uint16_t StackSize = 300;
    Statemachine(Canopen &co, RemoteControl &rc, HardwareSwitches &hws, LEDUpdater &ledU,
                 TerminalIO &term, wrapper::HAL &hal, IWDG_HandleTypeDef &iwdg, CanFestivalTimers& cft);
    virtual ~Statemachine() = default;

    Statemachine(const Statemachine &) = delete;
    Statemachine(Statemachine &&) = delete;
    Statemachine &operator=(const Statemachine &) = delete;
    Statemachine &operator=(Statemachine &&) = delete;

    StateId getCurrentState() const
    {
        return _currentState;
    }

    static const char *getStateName(const CanDeviceState);

    /**
     * @brief Internal. Called periodically by the associated task.
     *
     */
    virtual void dispatch();
    virtual void _dispatch(const std::span<const State> &, const State &idle);

    /**
     * @brief Replaces all logging with UI that shows device information and states
     *
     */
    virtual void drawUI() final;
    static constexpr uint32_t UI_UPDATE_TIME_MS{2000};

private:
    wrapper::Task _task;
    Canopen &_canopen;
    RemoteControl &_remoteControl;
    HardwareSwitches &_hwSwitches;
    LEDUpdater &_ledUpdater;
    TerminalIO& _terminalIO;
    wrapper::HAL& _hal;
    IWDG_HandleTypeDef &_iwdg;
    CanFestivalTimers& _cft;

    BusDevicesState _busDevicesState;
    RemoteControlState _remoteControlState;
    HardwareSwitchesState _hardwareSwitchesState;
    StateChaningSources _stateChaningSources;
    bool _startedUp = false;
    StateId _currentState = StateId::NO_STATE;
    States _states;

    static void taskMain(void* instance);
};
} // namespace remote_control_device
