#include "Logging.hpp"
#include "mock/HALMock.hpp"
#include "mock/TerminalIOMock.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <iostream>
#include <sstream>
#include <stm32f3xx_hal.h>

using namespace remote_control_device;
using ::testing::StrEq;
using ::testing::Return;

class LoggingTest : public ::testing::Test
{
protected:
    LoggingTest() : termIO(hal), logg(termIO, hal)
    {
    }

    HALMock hal;
    TerminalIOMock termIO;
    Logging logg;
};

std::string constructTimeoutMessage(int repeats)
{
    std::stringstream ss;
    ss << "... was repeated " << repeats << " more times the last "
       << static_cast<int>(Logging::REPEAT_MSG_TIMEOUT_SEC) << " second(s)\r\n";
    return ss.str();
}

TEST_F(LoggingTest, formatting)
{
    ON_CALL(hal, GetTick).WillByDefault(Return(0));

    EXPECT_CALL(termIO, write(StrEq("[ERROR][Bus Devices] Test1 123\r\n")));
    logg.log(Logging::Origin::BusDevices, Logging::Level::Error, "Test1 %d", 123);

    EXPECT_CALL(termIO, write(StrEq("[INFO][CanFestival] Test2 3.1\r\n")));
    logg.log(Logging::Origin::CanFestival, Logging::Level::Info, "Test2 %1.1f", 3.14);
}

TEST_F(LoggingTest, repeatMessageWithTimeout)
{
    static constexpr uint32_t Repeats = 100;
    uint32_t time = 0;

    // first instance of message should be printed out
    EXPECT_CALL(termIO, write(StrEq("[ERROR][Bus Devices] Test1 123\r\n"))).Times(1);

    EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(time++));
    logg.log(Logging::Origin::BusDevices, Logging::Level::Error, "Test1 %d", 123);
    for (size_t i = 0; i < 100; ++i)
    {
        EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(time++));
        logg.log(Logging::Origin::BusDevices, Logging::Level::Error, "Test1 %d", 123);

        // repeats > 0, so get tick is actually called
        EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(time++));
        logg.writeRepeatMessageAfterTimeout();
    }

    // no messages for long time, repeated message is printed after timeout
    EXPECT_CALL(termIO, write(StrEq(constructTimeoutMessage(100))));
    EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(time + Logging::REPEAT_MSG_TIMEOUT_SEC * 1000 + 1));
    logg.writeRepeatMessageAfterTimeout();
}

TEST_F(LoggingTest, repeatMessageWithNewMessage)
{
    ON_CALL(hal, GetTick).WillByDefault(Return(0));
    static constexpr uint32_t Repeats = 2;

    // first instance of message should be printed out
    EXPECT_CALL(termIO, write(StrEq("[ERROR][Bus Devices] Test1 123\r\n"))).Times(1);
    logg.log(Logging::Origin::BusDevices, Logging::Level::Error, "Test1 %d", 123);
    logg.log(Logging::Origin::BusDevices, Logging::Level::Error, "Test1 %d", 123);

    // message repeats
    EXPECT_CALL(termIO, write(StrEq(constructTimeoutMessage(1)))).Times(1);
    EXPECT_CALL(termIO, write(StrEq("[ERROR][Bus Devices] Test2 456\r\n"))).Times(1);
    logg.log(Logging::Origin::BusDevices, Logging::Level::Error, "Test2 %d", 456);
}