#include "CanFestival/CanFestivalTimers.hpp"
#include "mock/LoggingMock.hpp"
#include "gtest/gtest.h"
#include <CanFestival/CanFestivalLocker.hpp>
#include <array>
#include <hippomocks.h>
#include <iostream>
#include <stm32f3xx_hal.h>

extern "C"
{
#include <canfestival/data.h>
}

using namespace remote_control_device;
using ::testing::Return;

class CanFestivalTimersTest : public ::testing::Test
{
protected:
    CanFestivalTimersTest() : term(halMock), log(term, halMock), cft(halMock, log)
    {
    }

    HALMock halMock;
    TerminalIOMock term;
    LoggingMock log;
    CanFestivalTimers cft;
};

TEST_F(CanFestivalTimersTest, delAlarmWrongParameters)
{
    MockRepository mocks;

    // Test parameter checking
    mocks.ExpectCallFunc(CanFestivalTimers::testHook_ErrorDelAlarm);
    EXPECT_EQ(cft.delAlarm(CanFestivalTimers::MAX_TIMERS), TIMER_NONE);

    mocks.ExpectCallFunc(CanFestivalTimers::testHook_ErrorDelAlarm);
    EXPECT_EQ(cft.delAlarm(TIMER_NONE - 1), TIMER_NONE);
}

TEST_F(CanFestivalTimersTest, setAlarmWrongParameters)
{
    MockRepository mocks;

    // Test parameter checking
    mocks.ExpectCallFunc(CanFestivalTimers::testHook_ErrorSetAlarm);
    EXPECT_EQ(
        cft.setAlarm(
            nullptr, 0, [](CO_Data *d, UNS32 id) -> void {}, MS_TO_TIMEVAL(0), MS_TO_TIMEVAL(0)),
        TIMER_NONE);

    CFLocker locker;
    mocks.ExpectCallFunc(CanFestivalTimers::testHook_ErrorSetAlarm);
    EXPECT_EQ(cft.setAlarm(locker.getOD(), 0, nullptr, MS_TO_TIMEVAL(0), MS_TO_TIMEVAL(0)),
              TIMER_NONE);
}

TEST_F(CanFestivalTimersTest, setAlarmSaturationDispatch)
{
    MockRepository mocks;
    std::array<bool, CanFestivalTimers::MAX_TIMERS> callbackResult = {false};

    // test filling up alarm array
    for (size_t i = 0; i < CanFestivalTimers::MAX_TIMERS; ++i)
    {
        TIMER_HANDLE res;
        // missusing the CO_Data field to pass the result array,  ugly but it works in this context
        // one could bind a parameter via std::function but it
        // is a beast to use on embedded and might require a heap manager with delete
        // support (depends on the size of the function wrapped in it)
        EXPECT_CALL(halMock, GetTick).WillOnce(Return(0));
        res = cft.setAlarm(
            reinterpret_cast<CO_Data *>(&callbackResult), i,
            [](CO_Data *d, UNS32 id) {
                auto result = reinterpret_cast<
                    std::array<bool, remote_control_device::CanFestivalTimers::MAX_TIMERS> *>(d);

                // called with valid index
                ASSERT_LT(id, result->size());
                ASSERT_GE(id, 0);

                // not already called before
                ASSERT_EQ(result->at(id), false);

                result->at(id) = true;
                return;
            },
            MS_TO_TIMEVAL(i), MS_TO_TIMEVAL(0));

        ASSERT_NE(res, TIMER_NONE);
    }

    // must fail, no space available
    ASSERT_EQ(cft.setAlarm((CO_Data *)1, 0, [](CO_Data *d, UNS32 id) { return; }, MS_TO_TIMEVAL(0),
                           MS_TO_TIMEVAL(0)),
              TIMER_NONE);

    // test dispatching all timers one after another
    for (size_t i = 0; i < CanFestivalTimers::MAX_TIMERS; ++i)
    {
        EXPECT_CALL(halMock, GetTick).WillOnce(Return(i));
        ASSERT_EQ(cft.dispatch(), 1);
        ASSERT_EQ(callbackResult.at(i), true);

        // no other timers called yet
        for (size_t j = i + 1; j < CanFestivalTimers::MAX_TIMERS; ++j)
        {
            ASSERT_EQ(callbackResult.at(j), false);
        }
    }

    // test timers not dispatched again
    for (bool &e : callbackResult)
    {
        e = false;
    }
    EXPECT_CALL(halMock, GetTick).WillOnce(Return(100));
    ASSERT_GE(cft.dispatch(), 1); // more waittime than minimum
    for (bool &e : callbackResult)
    {
        ASSERT_EQ(e, false);
    }
}

#define ID 420
TEST_F(CanFestivalTimersTest, setAlarmPeriodicDispatchAndDelete)
{
    const int firstDispatchIn = 10;
    const int periodicTime = 100;
    int calledCount = 0;

    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(0));

    // dirty trick again see setAlarmSaturationDispatch
    TIMER_HANDLE handle;
    handle = cft.setAlarm(
        reinterpret_cast<CO_Data *>(&calledCount), ID,
        [](CO_Data *d, UNS32 id) {
            auto value = reinterpret_cast<int *>(d);
            ++(*value);
            ASSERT_EQ(id, ID);
            return;
        },
        MS_TO_TIMEVAL(firstDispatchIn), MS_TO_TIMEVAL(periodicTime));

    ASSERT_NE(handle, TIMER_NONE);

    // dispatch couple of times
    EXPECT_CALL(halMock, GetTick).WillOnce(Return(firstDispatchIn / 2));
    cft.dispatch();
    ASSERT_EQ(calledCount, 0);

    EXPECT_CALL(halMock, GetTick).WillOnce(Return(firstDispatchIn));
    cft.dispatch();
    ASSERT_EQ(calledCount, 1);

    EXPECT_CALL(halMock, GetTick).WillOnce(Return(firstDispatchIn + (periodicTime / 2)));
    cft.dispatch();
    ASSERT_EQ(calledCount, 1);

    EXPECT_CALL(halMock, GetTick).WillOnce(Return(firstDispatchIn + periodicTime));
    cft.dispatch();
    ASSERT_EQ(calledCount, 2);

    // delete
    ASSERT_EQ(cft.delAlarm(handle), TIMER_NONE);

    EXPECT_CALL(halMock, GetTick).WillOnce(Return(firstDispatchIn + (periodicTime * 2)));
    cft.dispatch();
    ASSERT_EQ(calledCount, 2);

    EXPECT_CALL(halMock, GetTick).WillOnce(Return(firstDispatchIn + (periodicTime * 3)));
    cft.dispatch();
    ASSERT_EQ(calledCount, 2);
}

TEST_F(CanFestivalTimersTest, bug32BitCurrentTimeOverflow)
{
    /**
     * This is a bug where the current timestamp (ms steps) is multiplied by 1000 to be consistent
     * with TIMEVAL being in µs. After 71 minutes an overflow occurs when the (still 32 bit) variable
     * is multiplied by 1000 and then written into a 64 bit variable causing overflows in multiple places
     * which eventually leads to a locked up CFT task
     */
    unsigned int time = 4'234'967; // 70 minutes in milliseconds
    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));

    auto increaserCallback = [](CO_Data *d, UNS32 id) {
        auto value = reinterpret_cast<int *>(d);
        ++(*value);
    };

    TIMER_HANDLE handle;
    int cntr = 0;
    handle = cft.setAlarm(reinterpret_cast<CO_Data *>(&cntr), ID, increaserCallback,
                          MS_TO_TIMEVAL(1), MS_TO_TIMEVAL(100));
    ASSERT_NE(handle, TIMER_NONE);
    time++;

    for (int i=0;i<10000;++i) {
        time += 100;
        EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
        ASSERT_EQ(cft.dispatch(), 1); // one is returned when timers successfully fired
        ASSERT_EQ(cntr, i+1); // check if really fired
        // with the bug in place this will fail at iteration 600 - the missing minute to 71
    }
}