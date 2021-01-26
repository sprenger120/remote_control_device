#include "mock/CanIOMock.hpp"
#include "mock/CanopenMock.hpp"
#include "mock/TerminalIOMock.hpp"
#include "mock/LEDsMock.hpp"
#include "mock/HALMock.hpp"
#include "gtest/gtest.h"
#include <FreeRTOS.h>
#include <Statemachine/LEDUpdater.hpp>
#include <array>
#include <exception>
#include <iostream>
#include <stm32f3xx_hal.h>
#include <stm32f3xx_hal_can.h>
#include <task.h>

using ::testing::Return;
using namespace remote_control_device;

class LEDUpdaterTest : public ::testing::Test
{
protected:
    LEDUpdaterTest()
        : term(hal), log(term, hal), canIO(hal, log), ledU(ledHw, ledRc, canIO), co(canIO, log),
          currState(StateId::NO_STATE), startup(false),
          scs(currState, co, startup, busDevSate, rcState, hws)
    {
    }

    HALMock hal;
    TerminalIOMock term;
    LoggingMock log;
    CanIOMock canIO;
    LEDMock ledHw;
    LEDMock ledRc;
    LEDUpdater ledU;
    CanopenMock co;

    StateId currState;
    BusDevicesState busDevSate;
    RemoteControlState rcState;
    HardwareSwitchesState hws;
    bool startup;
    StateChaningSources scs;
};

TEST_F(LEDUpdaterTest, update_AllOK)
{
    rcState.timeout = false;
    hws.ManualSwitch = false;
    startup = true;

    // everything OK
    // testing general functionality
    EXPECT_CALL(canIO, isBusOK).WillRepeatedly(Return(true));

    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::DriveMotorController))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::SteeringActuator))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::BrakeActuator))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(ledHw, setMode(LED::Mode::On, 0)).Times(1);
    EXPECT_CALL(ledRc, setMode(LED::Mode::On, 0)).Times(1);
    ledU.update(scs);
}

TEST_F(LEDUpdaterTest, update_SomeError)
{
    // remote timeout, one bus devices missing
    rcState.timeout = true;
    hws.ManualSwitch = false;
    startup = true;

    EXPECT_CALL(canIO, isBusOK).WillRepeatedly(Return(true));

    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::DriveMotorController))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::SteeringActuator))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::BrakeActuator))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(ledHw, setMode(LED::Mode::Counting, 3)).Times(1);
    EXPECT_CALL(ledRc, setMode(LED::Mode::Off, 0)).Times(1);
    ledU.update(scs);
}

TEST_F(LEDUpdaterTest, update_MultiError)
{
    rcState.timeout = true;
    hws.ManualSwitch = true;
    startup = true;

    // remote timeout, manual mode,  one bus devices missing, can broken
    // test priorisation
    EXPECT_CALL(canIO, isBusOK).WillRepeatedly(Return(false));
    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::DriveMotorController))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::SteeringActuator))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(co, isDeviceOnline(Canopen::BusDevices::BrakeActuator))
        .WillRepeatedly(Return(false));

    EXPECT_CALL(ledHw, setMode(LED::Mode::Counting, 1)).Times(1);
    EXPECT_CALL(ledRc, setMode(LED::Mode::FastBlink, 0)).Times(1);
    ledU.update(scs);
}