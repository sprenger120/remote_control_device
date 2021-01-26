#include "TestDataSBUSFrame.hpp"
#include "gtest/gtest.h"

using namespace remote_control_device::SBUS;
using namespace TestDataSBUSFrame;

TEST(SBUSDecoderTest, decode_GoodFrame)
{
    const auto result = Decoder::decode(GoodFrame::frameData);
    const auto & targetFrame = result.second;

    EXPECT_EQ(result.first, DecodeError::NoError);
    for (size_t i = 0; i < Protocol::ANALOG_CHANNEL_COUNT; ++i)
    {
        EXPECT_EQ(targetFrame.analogChannels[i],
                  Decoder::scaleRawValue(GoodFrame::RawAnalogChannelValue));
    }
    EXPECT_EQ(targetFrame.digitalCh17, GoodFrame::Ch17);
    EXPECT_EQ(targetFrame.digitalCh18, GoodFrame::Ch18);
    EXPECT_EQ(targetFrame.frameLost, GoodFrame::FrameLost);
    EXPECT_EQ(targetFrame.failsafe, GoodFrame::Failsafe);
}

TEST(SBUSDecoderTest, decode_GoodFrameTimeout)
{
    const auto result = Decoder::decode(GoodFrameTimeout::frameData);
    const auto & targetFrame = result.second;

    EXPECT_EQ(result.first, DecodeError::NoError);
    for (size_t i = 0; i < Protocol::ANALOG_CHANNEL_COUNT; ++i)
    {
        EXPECT_EQ(targetFrame.analogChannels[i], Decoder::ON_FAILSAFE_ANALOG_CHANNEL_VALUE);
    }
    EXPECT_EQ(targetFrame.digitalCh17, Decoder::ON_FAILSAFE_DIGITAL_CHANNEL_VALUE);
    EXPECT_EQ(targetFrame.digitalCh18, Decoder::ON_FAILSAFE_DIGITAL_CHANNEL_VALUE);
    EXPECT_EQ(targetFrame.frameLost, GoodFrameTimeout::FrameLost);
    EXPECT_EQ(targetFrame.failsafe, true);
}

TEST(SBUSDecoderTest, decode_BadFrameStartByte_FailsafeChannelValues)
{
    const auto result = Decoder::decode(BadFrameStartByte::frameData);
    const auto & targetFrame = result.second;
    EXPECT_NE(result.first, DecodeError::NoError);
    for (size_t i = 0; i < Protocol::ANALOG_CHANNEL_COUNT; ++i)
    {
        EXPECT_EQ(targetFrame.analogChannels[i], Decoder::ON_FAILSAFE_ANALOG_CHANNEL_VALUE);
    }
    EXPECT_EQ(targetFrame.digitalCh17, Decoder::ON_FAILSAFE_DIGITAL_CHANNEL_VALUE);
    EXPECT_EQ(targetFrame.digitalCh18, Decoder::ON_FAILSAFE_DIGITAL_CHANNEL_VALUE);
    EXPECT_EQ(targetFrame.frameLost, GoodFrameTimeout::FrameLost);
    EXPECT_EQ(targetFrame.failsafe, true);
}

TEST(SBUSDecoderTest, decode_VariousBadFrames)
{
    EXPECT_NE(Decoder::decode(BadFrameStartByte::frameData).first, DecodeError::NoError);
    EXPECT_NE(Decoder::decode(BadFrameEndByte::frameData).first, DecodeError::NoError);
    EXPECT_NE(Decoder::decode(BadFrameFlagBytes::frameData).first, DecodeError::NoError);
    EXPECT_NE(Decoder::decode(BadFrameChannelValue::frameData).first, DecodeError::NoError);
    EXPECT_NE(Decoder::decode(BadFrameChannelValue2::frameData).first, DecodeError::NoError);
}