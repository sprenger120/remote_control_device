#include "LEDUpdater.hpp"
#include "Canopen.hpp"
#include "Statemachine.hpp"
#include "LEDs.hpp"
#include "PeripheralDrivers/CanIO.hpp"

namespace remote_control_device
{

LEDUpdater::LEDUpdater(LED &ledHw, LED &ledRC, CanIO &canio)
    : _ledHw(ledHw), _ledRC(ledRC), _canIO(canio)
{
}

void LEDUpdater::update(StateChaningSources &src)
{
    LED::Mode finalMode = LED::Mode::Off;
    int finalCount = 0;

    // Update rc led
    if (src.remoteControl.timeout)
    {
        finalMode = LED::Mode::Off;
    }
    else
    {
        finalMode = LED::Mode::On;
    }
    if (src.hardwareSwitches.ManualSwitch)
    {
        finalMode = LED::Mode::FastBlink;
    }
    _ledRC.setMode(finalMode, finalCount);

    // update Hardware LED
    finalMode = LED::Mode::On;
    finalCount = 0;

    if (!src.canopen.isDeviceOnline(Canopen::BusDevices::DriveMotorController))
    {
        finalMode = LED::Mode::Counting;
        finalCount = 4;
    }
    if (!src.canopen.isDeviceOnline(Canopen::BusDevices::SteeringActuator))
    {
        finalMode = LED::Mode::Counting;
        finalCount = 3;
    }
    if (!src.canopen.isDeviceOnline(Canopen::BusDevices::BrakeActuator))
    {
        finalMode = LED::Mode::Counting;
        finalCount = 2;
    }
    if (!_canIO.isBusOK()) {
        finalMode = LED::Mode::Counting;
        finalCount = 1;
    }
    if (!src.isStartedUp()) {
        finalMode = LED::Mode::FastBlink;
        finalCount = 0;
    }
    _ledHw.setMode(finalMode, finalCount);
}

} // namespace remote_control_device