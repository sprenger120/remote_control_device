#pragma once
#include "SBUSDecoder.hpp"
#include <FreeRTOS.h>

using namespace remote_control_device::SBUS;

namespace TestDataSBUSFrame
{
// Don't change any values unless you also change the frameData

struct GoodFrame
{
static constexpr uint16_t RawAnalogChannelValue = 500;
static constexpr bool Failsafe = false;
static constexpr bool FrameLost = false;
static constexpr bool Ch18 = true;
static constexpr bool Ch17 = true;
static Protocol::FrameData frameData;
}; 

struct GoodFrameTimeout
{
static constexpr uint16_t RawAnalogChannelValue = 500;
static constexpr bool Failsafe = false;
static constexpr bool FrameLost = true;
static constexpr bool Ch18 = true;
static constexpr bool Ch17 = true;
static Protocol::FrameData frameData;
}; 

struct BadFrameStartByte
{
static constexpr uint16_t RawAnalogChannelValue = 500;
static constexpr bool Failsafe = false;
static constexpr bool FrameLost = true;
static constexpr bool Ch18 = true;
static constexpr bool Ch17 = true;
static Protocol::FrameData frameData;
}; 

struct BadFrameEndByte
{
static constexpr uint16_t RawAnalogChannelValue = 500;
static constexpr bool Failsafe = false;
static constexpr bool FrameLost = true;
static constexpr bool Ch18 = true;
static constexpr bool Ch17 = true;
static Protocol::FrameData frameData;
}; 

struct BadFrameFlagBytes
{
static constexpr uint16_t RawAnalogChannelValue = 500;
static constexpr bool Failsafe = false;
static constexpr bool FrameLost = true;
static constexpr bool Ch18 = true;
static constexpr bool Ch17 = true;
static Protocol::FrameData frameData;
}; 

struct BadFrameChannelValue
{
static constexpr uint16_t RawAnalogChannelValue = 500;
static constexpr bool Failsafe = false;
static constexpr bool FrameLost = true;
static constexpr bool Ch18 = true;
static constexpr bool Ch17 = true;
static Protocol::FrameData frameData;
}; 

struct BadFrameChannelValue2
{
static constexpr uint16_t RawAnalogChannelValue = 500;
static constexpr bool Failsafe = false;
static constexpr bool FrameLost = true;
static constexpr bool Ch18 = true;
static constexpr bool Ch17 = true;
static Protocol::FrameData frameData;
}; 

} // namespace TestDataSBUSFrame