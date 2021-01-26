#include "Statemachine.hpp"
#include "ANSIEscapeCodes.hpp"
#include "CanFestival/CanFestivalTimers.hpp"
#include "Canopen.hpp"
#include "HardwareSwitches.hpp"
#include "LEDUpdater.hpp"
#include "Logging.hpp"
#include "PeripheralDrivers/CanIO.hpp"
#include "PeripheralDrivers/TerminalIO.hpp"
#include "RemoteControl.hpp"
#include "SpecialAssert.hpp"
#include "States.hpp"
#include "Wrapper/Sync.hpp"
#include <base/build_information.hpp>
#include <cmsis_os2.h>
#include <cstdio>
#include <functional>
#include <memory>

namespace remote_control_device
{

StateChaningSources::StateChaningSources(const StateId &state, Canopen &co, bool &startedUp,
                                         const BusDevicesState &busDevicesState,
                                         const RemoteControlState &remoteControl,
                                         const HardwareSwitchesState &hardwareSwitches)
    : busDevicesState(busDevicesState), remoteControl(remoteControl),
      hardwareSwitches(hardwareSwitches), currentState(state), canopen(co), _startedUp(startedUp)
{
}
void StateChaningSources::sigalStartedUp()
{
    _startedUp = true;
}
bool StateChaningSources::isStartedUp() const
{
    return _startedUp;
}

Statemachine::Statemachine(Canopen &co, RemoteControl &rc, HardwareSwitches &hws, LEDUpdater &ledU,
                           TerminalIO &term, wrapper::HAL &hal, IWDG_HandleTypeDef &iwdg,
                           CanFestivalTimers &cft)
    : _task(&Statemachine::taskMain, "Statemachine", StackSize, reinterpret_cast<void *>(this),
            osPriority_t::osPriorityNormal, wrapper::sync::Statemachine_Ready),
      _canopen(co), _remoteControl(rc), _hwSwitches(hws), _ledUpdater(ledU), _terminalIO(term),
      _hal(hal), _iwdg(iwdg), _cft(cft),
      _stateChaningSources(_currentState, _canopen, _startedUp, _busDevicesState,
                           _remoteControlState, _hardwareSwitchesState)
{
}

void Statemachine::dispatch()
{
    _dispatch(std::span(_states.get().data(), _states.get().size()), _states.getIdleState());
}

void Statemachine::_dispatch(const std::span<const State> &states, const State &idle)
{
    // update state changing sources
    _canopen.update(_busDevicesState);
    _remoteControl.update(_remoteControlState);
    _hwSwitches.update(_hardwareSwitchesState);

    // to through states and select highest that has its conditions fulfilled
    auto refSelectedState = std::cref(idle);
    for (const auto &state : states)
    {
        auto condition = state.callbacks->checkConditions(_stateChaningSources);

        if (condition && state.priority >= refSelectedState.get().priority &&
            ((state.requiresUnlock && _stateChaningSources.remoteControl.switchUnlock) ||
             !state.requiresUnlock || _currentState == state.id))
        {
            refSelectedState = std::cref(state);
        }
    }

    const auto &selectedState = refSelectedState.get();

    // state switch
    if (selectedState.id != _currentState)
    {
        selectedState.callbacks->oneTimeSetup(_stateChaningSources);

        _terminalIO.getLogging().logInfo(
            Logging::Origin::StateMachine, "Switched to task %s from %s",
            getStateIdName(selectedState.id), getStateIdName(_currentState));
        _currentState = selectedState.id;

        _canopen.setSelfState(_currentState);
        _canopen.setActuatorPDOs(selectedState.sendTargets);
        _canopen.setDeviceState(Canopen::BusDevices::SteeringActuator, selectedState.stateSteering);
        _canopen.setDeviceState(Canopen::BusDevices::BrakeActuator, selectedState.stateBrake);
        _canopen.setDeviceState(Canopen::BusDevices::DriveMotorController,
                                selectedState.stateDriveMotor);
        _canopen.setDeviceState(Canopen::BusDevices::BrakePressureSensor,
                                CanDeviceState::Operational);
        _canopen.setCouplingStates(selectedState.couplingBrake == CouplingState::Engaged,
                                   selectedState.couplingSteering == CouplingState::Engaged);
    }

    // execute
    selectedState.callbacks->process(_stateChaningSources);
    _ledUpdater.update(_stateChaningSources);
}

void Statemachine::drawUI()
{
    TerminalIO &term = _terminalIO;
    term.getLogging().disableLogging();

    term.write(ANSIEscapeCodes::ClearTerminal);
    term.write(ANSIEscapeCodes::MoveCursorToHome);

    // Header
    term.write(ANSIEscapeCodes::ColorSection_WhiteText_MagentaBackground);
    term.write("Remote-Control-Device (RCD)\r\n\r\n");
    term.write(ANSIEscapeCodes::ColorSection_End);

    // Buildinfo
    term.write(ANSIEscapeCodes::ColorSection_WhiteText_GrayBackground);
    term.write("Build information:\r\n");
    term.write(ANSIEscapeCodes::ColorSection_End);
    term.write("Git Hash: ");
    term.write(build_info::CommitHashLong);
    term.write("\r\nBranch: ");
    term.write(build_info::BranchName);

    term.write("\r\nDebug build: ");
    if (build_info::IsDebugBuild)
    {
        term.write("Yes\r\n");
    }
    else
    {
        term.write("No\r\n");
    }
    term.write("Compiled with uncommited changes: ");
    if (build_info::IsDirty)
    {
        term.write("Yes\r\n");
    }
    else
    {
        term.write("No\r\n");
    }

    // Statemachine
    term.write(ANSIEscapeCodes::ColorSection_WhiteText_GrayBackground);
    term.write("\r\nStatemachine:\r\n");
    term.write(ANSIEscapeCodes::ColorSection_End);

    term.write("Current State:");
    term.write(getStateIdName(_currentState));
    term.write("\r\n");

    term.write("Remote Control Connection: ");
    if (_remoteControlState.timeout)
    {
        term.write(ANSIEscapeCodes::ColorSection_WhiteText_RedBackground);
        term.write("Timeout\r\n");
        term.write(ANSIEscapeCodes::ColorSection_End);
    }
    else
    {
        term.write("Online\r\n");
    }

    term.write("Manual Switch: ");
    if (_hardwareSwitchesState.ManualSwitch)
    {
        term.write(ANSIEscapeCodes::ColorSection_BlackText_GreenBackground);
        term.write("Active\r\n");
        term.write(ANSIEscapeCodes::ColorSection_End);
    }
    else
    {
        term.write("Inactive\r\n");
    }

    term.write("Bike Emergency Switch: ");
    if (_hardwareSwitchesState.BikeEmergency)
    {
        term.write(ANSIEscapeCodes::ColorSection_BlackText_YellowBackground);
        term.write("Active\r\n");
        term.write(ANSIEscapeCodes::ColorSection_End);
    }
    else
    {
        term.write("Inactive\r\n");
    }

    // Statemachine
    term.write(ANSIEscapeCodes::ColorSection_WhiteText_GrayBackground);
    term.write("\r\nSystem:\r\n");
    term.write(ANSIEscapeCodes::ColorSection_End);

    static constexpr size_t buffSize = 20;
    char buff[buffSize] = {0};
    uint8_t timersRemain = _cft.getTimersRemaining();
    snprintf(buff, buffSize, "%d", timersRemain);
    term.write("Remaining Timer Slots: ");
    term.write(buff);
    term.write("\r\n");

    snprintf(buff, buffSize, "%lu", (_hal.GetTick() / 1000));
    term.write("Ontime: ");
    term.write(buff);
    term.write("(s)\r\n");

    uint16_t ramRemain = xPortGetFreeHeapSize();
    snprintf(buff, buffSize, "%d", ramRemain);
    term.write("Free Memory ");
    term.write(buff);
    term.write(" bytes\r\n");

    term.write("Tasks at max stack; words (4 bytes) still free:\r\n");

    const auto &tasks = wrapper::Task::getAllTaskHandles();
    for (const auto &t : tasks)
    {
        if (t == nullptr)
        {
            continue;
        }
        term.write("\t");
        term.write(pcTaskGetName(t));
        term.write(": ");

        UBaseType_t words = uxTaskGetStackHighWaterMark(t);
        snprintf(buff, buffSize, "%lu", words);
        term.write(buff);
        term.write("\r\n");
    }

    // Monitored Devices, state controlled devices
    _canopen.drawUIDevicesPart(term);
}

void Statemachine::taskMain(void *instance)
{
    specialAssert(instance != nullptr);
    auto sm = reinterpret_cast<Statemachine *>(instance);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t lastDraw = 0;
    for (;;)
    {
        HAL_IWDG_Refresh(&(sm->_iwdg));

        sm->dispatch();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(20));
        // stackPrinter.print();
        // TerminalIO::instance().log().writeRepeatMessageAfterTimeout();
        if (sm->_hal.GetTick() - lastDraw > Statemachine::UI_UPDATE_TIME_MS)
        {
            lastDraw = sm->_hal.GetTick();
            sm->drawUI();
        }
    }
}

} // namespace remote_control_device
