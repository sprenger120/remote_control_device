#pragma once
#include "BuildConfiguration.hpp"
#include "Wrapper/HAL.hpp"
#include "CanFestival/CanFestivalTimers.hpp"
#include "LEDs.hpp"
#include "PeripheralDrivers/CanIO.hpp"
#include "PeripheralDrivers/ReceiverModule.hpp"
#include "PeripheralDrivers/TerminalIO.hpp"
#include "Statemachine/Canopen.hpp"
#include "Statemachine/HardwareSwitches.hpp"
#include "Statemachine/LEDUpdater.hpp"
#include "Statemachine/RemoteControl.hpp"
#include "Statemachine/Statemachine.hpp"

#ifdef BUILDCONFIG_EMBEDDED_BUILD
namespace remote_control_device
{
class Application
{
public:
    Application();

    void run();
private:
    wrapper::HAL _hal;
    TerminalIO _terminalIO;
    ReceiverModule _receiverModule;
    CanIO _canIO;
    CanFestivalTimers _cft;

    Canopen _canOpen;
    RemoteControl _remoteControl;
    HardwareSwitches _hardwareSwitches;
    LED _ledHw;
    LED _ledRc;
    LEDUpdater _ledUpdater;
    Statemachine _stateMachine;
};
} // namespace remote_control_device
#endif