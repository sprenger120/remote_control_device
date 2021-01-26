#pragma once
#include "gmock/gmock.h"
#include <LEDs.hpp>

using namespace remote_control_device;

class LEDMock : public LED
{
public:
    LEDMock() : LED(nullptr, 0, false) {};
    MOCK_METHOD(void, setMode, (const LED::Mode, const uint8_t count), (override));
    MOCK_METHOD(TickType_t, dispatch, (), (override));
};