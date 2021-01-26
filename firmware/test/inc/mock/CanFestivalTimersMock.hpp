#pragma once
#include <CanFestival/CanFestivalTimers.hpp>
#include "mock/HALMock.hpp"
#include "mock/LoggingMock.hpp"

using namespace remote_control_device;



class CanFestivalTimersMock : public CanFestivalTimers
{
public:
    CanFestivalTimersMock(HALMock& hal, LoggingMock& log) : CanFestivalTimers(hal, log)
    {
    }
    MOCK_METHOD(TickType_t, dispatch, (), (override));
    MOCK_METHOD(void, taskMain, (), (override));
    MOCK_METHOD(uint8_t, getTimersRemaining, (), (override));
};
