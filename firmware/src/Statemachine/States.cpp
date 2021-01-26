#include "States.hpp"
#include "Statemachine/Canopen.hpp"
#include "Statemachine/StateSources.hpp"
#include "Statemachine/Statemachine.hpp"

namespace remote_control_device
{

States::States()
    : // clang-format off
_states{{ 
        /* Start */
        State(
            /* id */ StateId::Start,
            /* priority */ 255,

            /* brake coupling */ CouplingState::Disengaged,
            /* steering coupling */ CouplingState::Disengaged,

            /* send targets */ false,
            /* steering status */ CanDeviceState::Operational, 
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ false,

            new StateCallbacks(
                /* process function*/
                [](StateChaningSources &src) -> void {
                    // not starting up until all devices are online and 
                    // rtd_state signals sucessful bootup
                    if (src.busDevicesState.rtdBootedUp && !src.busDevicesState.timeout) 
                    { 
                        src.sigalStartedUp();
                    }
                },
                /* check conditions */
                [](StateChaningSources &src) -> bool { 
                    return !src.isStartedUp(); 
                },
                /* one time setup */ 
                [](StateChaningSources &src) -> void { 
                }
            )
        ),

        /* Idle */
        State(
            /* id */ StateId::Idle,
            /* priority */ IDLE_STATE_PRIORITY,

            /* brake coupling */ CouplingState::Disengaged,
            /* steering coupling */ CouplingState::Disengaged,

            /* send targets */ false,
            /* steering status */ CanDeviceState::Preoperational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,
            
            /* requires unlock */ false,

            new StateCallbacks(
                /* process function*/
                [](StateChaningSources &src) -> void {
                    return;
                },
                /* check conditions */
                [](StateChaningSources &src) -> bool { 
                    return true; // gets replaced anyways
                },
                /* one time setup */ 
                [](StateChaningSources &src) -> void {
                    // reset brake, motor torque just once with enable->disable 
                    // see canopen:: setActuatorPDOs and kickstartPDOTransmission
                    src.canopen.setWheelDriveTorque(0);
                    src.canopen.setBrakeForce(0);
                    src.canopen.setActuatorPDOs(true);
                    src.canopen.setActuatorPDOs(false);
                }
            )
        ),

        /* RemoteControl */
        State(
            /* id */ StateId::RemoteControl,
            /* priority */ 11,

            /* brake coupling */ CouplingState::Engaged,
            /* steering coupling */ CouplingState::Engaged,

            /* send targets */ true,
            /* steering status */ CanDeviceState::Operational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ true,

            new StateCallbacks(
                /* process function*/
                [](StateChaningSources &src) -> void {
                    src.canopen.setBrakeForce(src.remoteControl.brake);
                    src.canopen.setWheelDriveTorque(src.remoteControl.throttle);
                    src.canopen.setSteeringAngle(src.remoteControl.steering);
                    return;
                },
                /* check conditions */
                [](StateChaningSources &src) -> bool { 
                    return  src.remoteControl.switchRemoteControl && (!src.remoteControl.switchAutonomous) && (!src.remoteControl.throttleIsUp);
                },
                /* one time setup */ 
                [](StateChaningSources &src) -> void { 
                }
            )
        ),

        /* Autonomous */
        State(
            /* id */ StateId::Autonomous,
            /* priority */ 10,

            /* brake coupling */ CouplingState::Engaged,
            /* steering coupling */ CouplingState::Engaged,

            /* send targets */ false,
            /* steering status */ CanDeviceState::Operational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ true,

            new StateCallbacks(
                /* process function*/
                [](StateChaningSources &src) -> void {
                    return;
                },
                /* check conditions */
                [](StateChaningSources &src) -> bool { 
                    return  src.remoteControl.switchAutonomous && (!src.remoteControl.switchRemoteControl);
                },
                /* one time setup */ 
                [](StateChaningSources &src) -> void { 
                }
            )
        ),

        /* SoftEmergency */
        State(
            /* id */ StateId::SoftEmergency,
            /* priority */ 100,

            /* brake coupling */ CouplingState::Engaged,
            /* steering coupling */ CouplingState::Engaged,

            /* send targets */ true,
            /* steering status */ CanDeviceState::Operational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ false,

            new StateCallbacks(
                /* process function*/
                [](StateChaningSources &src) -> void {
                    src.canopen.setBrakeForce(Canopen::MaxBrakePressure);
                    src.canopen.setWheelDriveTorque(Canopen::NoWheelDriveTorque);
                    return;
                },
                /* check conditions */
                [](StateChaningSources &src) -> bool { 
                    // keep in emergency until all remote switches are back to 
                    // initial position
                    bool latchUp = src.currentState == StateId::SoftEmergency &&
                            (src.remoteControl.switchRemoteControl ||
                            src.remoteControl.switchAutonomous ||
                            src.remoteControl.switchUnlock);

                    return  src.remoteControl.timeout || 
                            src.remoteControl.buttonEmergency ||
                            src.busDevicesState.timeout ||
                            src.busDevicesState.rtdEmergency ||
                            latchUp;  
                },
                /* one time setup */ 
                [](StateChaningSources &src) -> void { 
                }
            )
        ),

        /* Manual */
        State(
            /* id */ StateId::Manual,
            /* priority */ 250,

            /* brake coupling */ CouplingState::Disengaged,
            /* steering coupling */ CouplingState::Disengaged,

            /* send targets */ false,
            /* steering status */ CanDeviceState::Preoperational,
            /* brake status */ CanDeviceState::Preoperational,
            /* drive motor status */ CanDeviceState::Preoperational,

            /* requires unlock */ false,

             new StateCallbacks(
                /* process function*/   
                [](StateChaningSources &src) -> void {
                    return;
                },
                /* check conditions */
                [](StateChaningSources &src) -> bool { 
                    return src.hardwareSwitches.ManualSwitch;
                },
                /* one time setup */ 
                [](StateChaningSources &src) -> void { 
                    // reset brake, motor torque just once with enable->disable 
                    // see canopen:: setActuatorPDOs and kickstartPDOTransmission
                    src.canopen.setWheelDriveTorque(0);
                    src.canopen.setBrakeForce(0);
                    src.canopen.setActuatorPDOs(true);
                    src.canopen.setActuatorPDOs(false);
                }
            )
        ),

        /* Emergency */
        State(
            /* id */ StateId::Emergency,
            /* priority */ 150,

            /* brake coupling */ CouplingState::Engaged,
            /* steering coupling */ CouplingState::Engaged,

            /* send targets */ true,
            /* steering status */ CanDeviceState::Operational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ false,

            new StateCallbacks(
                /* process function*/
                [](StateChaningSources &src) -> void {
                    src.canopen.setBrakeForce(Canopen::MaxBrakePressure);
                    src.canopen.setWheelDriveTorque(Canopen::NoWheelDriveTorque);
                    return;
                },
                /* check conditions */
                [](StateChaningSources &src) -> bool { 
                    return src.hardwareSwitches.BikeEmergency;
                },
                /* one time setup */ 
                [](StateChaningSources &src) -> void { 
                }
            )
        )
    }},
      // clang-format on
      _idleState(_lookupIdleState())
{
    // checking for duplicate priorities
    // kind of cheap but it ensures that only one state can ever run
    for (const auto &e : _states)
    {
        for (const auto &i : _states)
        {
            if (std::addressof(e) != std::addressof(i) && e.priority == i.priority)
            {
                specialAssert(false);
                return;
            }
        }
    }

    // check that there is a priority zero state that can run when nothing else
    // is able to
    // also force it to always be able to run
    for (auto &e : _states)
    {
        if (e.priority == IDLE_STATE_PRIORITY)
        {
            e.callbacks->_checkConditions = [](StateChaningSources &) -> bool { return true; };
        }
    }
}

const State &States::_lookupIdleState() const
{
    for (const auto &e : _states)
    {
        if (e.priority == IDLE_STATE_PRIORITY)
        {
            return e;
        }
    }
    specialAssert(false); // no idle state
    return _states[0];    // never reached, satisfies compiler
}

} // namespace remote_control_device