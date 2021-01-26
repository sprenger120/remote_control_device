#include "Statemachine/RemoteControl.hpp"
#include "SBUSDecoder.hpp"
#include "mock/HALMock.hpp"
#include "mock/ReceiverModuleMock.h"
#include "mock/LoggingMock.hpp"
#include "mock/TerminalIOMock.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <FreeRTOS.h>
#include <Statemachine/StateSources.hpp>
#include <array>
#include <stm32f3xx_hal.h>

using namespace remote_control_device;
using ::testing::Return;

class RemoteControlTest : public ::testing::Test
{
protected:
    RemoteControlTest() : term(hal), log(term, hal), recv(hal, log), rc(hal, log, recv)
    {
    }

    void SetUp() override {
        EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(0));
    }

    RemoteControlState rcs;
    SBUS::Frame frame;
    HALMock hal;
    TerminalIOMock term;
    LoggingMock log;
    ReceiverModuleMock recv;
    RemoteControl rc;
};

TEST_F(RemoteControlTest, _update_Timeout)
{
    static constexpr uint32_t Magic_Time = RemoteControl::Timeout_Ms + 420;

    frame.lastUpdate = 0;
    frame.failsafe = false;

    // failsafe ok, time ok = no timeout
    rc._update(frame, rcs);
    EXPECT_FALSE(rcs.timeout);

    // failsafe!  time ok = timeout!
    frame.failsafe = true;
    rc._update(frame, rcs);
    EXPECT_TRUE(rcs.timeout);

    // failsafe ok,  time ok = no timeout
    frame.failsafe = false;
    rc._update(frame, rcs);
    EXPECT_FALSE(rcs.timeout);

    // timeout!
    EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(Magic_Time));
    rc._update(frame, rcs);
    EXPECT_TRUE(rcs.timeout);

    frame.lastUpdate = Magic_Time + 1;

    EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(Magic_Time + (RemoteControl::Timeout_Ms / 10)));
    rc._update(frame, rcs);
    EXPECT_FALSE(rcs.timeout);
}

TEST_F(RemoteControlTest, _update)
{
    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) =
        SBUS::Decoder::ANALOG_CHANNEL_MAX / 2;
    frame.analogChannels.at(RemoteControl::ChannelMap::Brake) = SBUS::Decoder::ANALOG_CHANNEL_MAX / 2;
    frame.analogChannels.at(RemoteControl::ChannelMap::Steering) =
        SBUS::Decoder::ANALOG_CHANNEL_MAX / 2;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchUnlock) =
        RemoteControl::SwitchOn_MinValue + 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchRemote) =
        RemoteControl::SwitchOn_MinValue + 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchAutonomous) =
        RemoteControl::SwitchOn_MinValue + 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::ButtonEmcy) =
        RemoteControl::SwitchOn_MinValue + 1;

    rc._update(frame, rcs);

    EXPECT_FLOAT_EQ(rcs.throttle, 0);
    EXPECT_FLOAT_EQ(rcs.brake, 0.5);
    EXPECT_FLOAT_EQ(rcs.steering, 0);
    EXPECT_TRUE(rcs.switchUnlock);
    EXPECT_TRUE(rcs.switchRemoteControl);
    EXPECT_TRUE(rcs.switchAutonomous);
    EXPECT_TRUE(rcs.buttonEmergency);
    EXPECT_FALSE(rcs.throttleIsUp);
}

TEST_F(RemoteControlTest, _update2)
{
    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = SBUS::Decoder::ANALOG_CHANNEL_MAX;
    frame.analogChannels.at(RemoteControl::ChannelMap::Brake) = SBUS::Decoder::ANALOG_CHANNEL_MAX;
    frame.analogChannels.at(RemoteControl::ChannelMap::Steering) = SBUS::Decoder::ANALOG_CHANNEL_MIN;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchUnlock) =
        RemoteControl::SwitchOn_MinValue - 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchRemote) =
        RemoteControl::SwitchOn_MinValue - 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchAutonomous) =
        RemoteControl::SwitchOn_MinValue - 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::ButtonEmcy) =
        RemoteControl::SwitchOn_MinValue - 1;

    rc._update(frame, rcs);

    EXPECT_FLOAT_EQ(rcs.throttle, 1);
    EXPECT_FLOAT_EQ(rcs.brake, 1);
    EXPECT_FLOAT_EQ(rcs.steering, -1);
    EXPECT_FALSE(rcs.switchUnlock);
    EXPECT_FALSE(rcs.switchRemoteControl);
    EXPECT_FALSE(rcs.switchAutonomous);
    EXPECT_FALSE(rcs.buttonEmergency);
    EXPECT_TRUE(rcs.throttleIsUp);
}

TEST_F(RemoteControlTest, throttleIsUp)
{
    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = SBUS::Decoder::ANALOG_CHANNEL_MAX;
    frame.analogChannels.at(RemoteControl::ChannelMap::Brake) = SBUS::Decoder::ANALOG_CHANNEL_MAX;
    frame.analogChannels.at(RemoteControl::ChannelMap::Steering) = SBUS::Decoder::ANALOG_CHANNEL_MIN;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchUnlock) =
        RemoteControl::SwitchOn_MinValue - 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchRemote) =
        RemoteControl::SwitchOn_MinValue - 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::SwitchAutonomous) =
        RemoteControl::SwitchOn_MinValue - 1;
    frame.analogChannels.at(RemoteControl::ChannelMap::ButtonEmcy) =
        RemoteControl::SwitchOn_MinValue - 1;

    rc._update(frame, rcs);
    EXPECT_TRUE(rcs.throttleIsUp);

    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = SBUS::Decoder::ANALOG_CHANNEL_MIN;
    rc._update(frame, rcs);
    EXPECT_TRUE(rcs.throttleIsUp);

    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = SBUS::Decoder::ANALOG_CHANNEL_MAX / 2;
    rc._update(frame, rcs);
    EXPECT_FALSE(rcs.throttleIsUp);

    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = (SBUS::Decoder::ANALOG_CHANNEL_MAX / 2) + RemoteControl::Deadband;
    rc._update(frame, rcs);
    EXPECT_FALSE(rcs.throttleIsUp);

    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = (SBUS::Decoder::ANALOG_CHANNEL_MAX / 2) - RemoteControl::Deadband;
    rc._update(frame, rcs);
    EXPECT_FALSE(rcs.throttleIsUp);

    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = (SBUS::Decoder::ANALOG_CHANNEL_MAX / 2) - RemoteControl::Deadband - 1;
    rc._update(frame, rcs);
    EXPECT_TRUE(rcs.throttleIsUp);

    frame.analogChannels.at(RemoteControl::ChannelMap::Throttle) = (SBUS::Decoder::ANALOG_CHANNEL_MAX / 2) + RemoteControl::Deadband + 1;
    rc._update(frame, rcs);
    EXPECT_TRUE(rcs.throttleIsUp);
}