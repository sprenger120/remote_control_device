#pragma once
#include "SpecialAssert.hpp"
#include "StateSources.hpp"
#include <FreeRTOS.h>
#include <memory>
#include <utility>

namespace remote_control_device
{
class StateChaningSources;

/**
 * @brief Internal states and their values for network communication
 *
 */
enum class StateId : uint8_t
{
    Start = 0,
    Idle = 1,
    RemoteControl = 2,
    Autonomous = 4,
    SoftEmergency = 5,
    Manual = 7,
    Emergency = 6,

    NO_STATE = 255
};
const char *getStateIdName(const StateId id);

class States;

class StateCallbacks
{
public:
    using ProcessFunc = auto (*)(StateChaningSources &) -> void;
    using CheckConditionsFunc = auto (*)(StateChaningSources &) -> bool;
    using OneTimeSetupFunc = auto (*)(StateChaningSources &) -> void;

    StateCallbacks(ProcessFunc pro, CheckConditionsFunc chk, OneTimeSetupFunc ots)
        : _process(pro), _checkConditions(chk), _oneTimeSetup(ots)
    {
        specialAssert(pro != nullptr && chk != nullptr && ots != nullptr);
    }
    virtual ~StateCallbacks() = default;

    StateCallbacks(const StateCallbacks &) = default;
    StateCallbacks(StateCallbacks &&) = default;
    StateCallbacks &operator=(const StateCallbacks &) = default;
    StateCallbacks &operator=(StateCallbacks &&) = default;

    virtual void process(StateChaningSources &src) const
    {
        _process(src);
    }
    virtual bool checkConditions(StateChaningSources &src) const
    {
        return _checkConditions(src);
    }
    virtual void oneTimeSetup(StateChaningSources &src) const
    {
        _oneTimeSetup(src);
    }

private:
    ProcessFunc _process;
    CheckConditionsFunc _checkConditions;
    OneTimeSetupFunc _oneTimeSetup;

    friend States;
};

struct State
{
    const StateId id;
    const uint8_t priority;

    const CouplingState couplingBrake;
    const CouplingState couplingSteering;

    const bool sendTargets;
    const CanDeviceState stateSteering;
    const CanDeviceState stateBrake;
    const CanDeviceState stateDriveMotor;

    const bool requiresUnlock;

    std::unique_ptr<StateCallbacks> callbacks;

    /**
     * @brief Construct a new State
     *
     * @param id StateId of this state
     * @param priority Priority value to tie-break multiple states
     * @param couplingBrake What state the brake coupling should be in this state
     * @param couplingSteering What state the steering coupling should be in this state
     * @param sendTargets If actuator's target values should be sent continously
     * @param stateSteering Target Canopen state of steering actuator
     * @param stateBrake Target Canopen state of brake actuator
     * @param stateDriveMotor Target Canopen state of drive motor
     * @param requiresUnlock If to get to this state Remote Control's unlock switch must be
     * active
     * @param StateCallbacks Heap allocated instance. Ownership will be taken. Contains
     * the "brain" of a state. What it does and when
     */
    State(StateId id, uint8_t priority, CouplingState couplingBrake, CouplingState couplingSteering,
          bool sendTargets, CanDeviceState stateSteering, CanDeviceState stateBrake,
          CanDeviceState stateDriveMotor, bool requiresUnlock, StateCallbacks *cb)
        : id(id), priority(priority), couplingBrake(couplingBrake),
          couplingSteering(couplingSteering), sendTargets(sendTargets),
          stateSteering(stateSteering), stateBrake(stateBrake), stateDriveMotor(stateDriveMotor),
          requiresUnlock(requiresUnlock), callbacks(cb)
    {
    }

    /**
     * @brief Overloading == for compile time checks of state array validity
     * Same id is illegal by design. Disallowing same priority is an easy way to
     * not have the situation where two states are able to run at the same time.
     *
     * @param other
     * @return true
     * @return false
     */
    constexpr bool operator==(const State &other) const
    {
        return id == other.id || priority == other.priority;
    }
};

} // namespace remote_control_device