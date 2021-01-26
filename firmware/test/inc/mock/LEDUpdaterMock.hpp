#pragma once
#include "mock/LEDsMock.hpp"
#include "mock/CanIOMock.hpp"
#include "gmock/gmock.h"
#include <LEDs.hpp>
#include <Statemachine/LEDUpdater.hpp>
#include <Statemachine/Statemachine.hpp>

using namespace remote_control_device;

class LEDUpdaterMock : public LEDUpdater
{
public:
    LEDUpdaterMock(CanIOMock &canio) : LEDUpdater(_ledHw, _ledRc, canio){};
    MOCK_METHOD(void, update, (StateChaningSources &), (override));

private:
    LEDMock _ledHw;
    LEDMock _ledRc;
};