#pragma once
#include <FreeRTOS.h>
#include <array>

namespace remote_control_device::SBUS::Protocol
{
static constexpr uint8_t StartByte = 0x0f;
static constexpr uint8_t EndByte = 0x00;

static constexpr uint8_t Mask_FlagByte_Ch17 = 1 << 0;
static constexpr uint8_t Mask_FlagByte_Ch18 = 1 << 1;
static constexpr uint8_t Mask_FlagByte_FrameLost = 1 << 2;
static constexpr uint8_t Mask_FlagByte_Failsafe = 1 << 3;
static constexpr uint8_t Mask_FlagByte_Empty = 0b11110000;

static constexpr uint8_t Analog_Channel_Bit_Width = 11;

// Values for FrSky QX7 radios
// Each remote control sends different raw ranges
// due to the nature of these radios, some clipping is still required
// internally these values are pulse widths in µs
static constexpr uint16_t Analog_Channel_RawQX7Range_Min = 172;
static constexpr uint16_t Analog_Channel_RawQX7Range_Max = 1811;
// Scaling QX7 values to 0-2000
static constexpr uint16_t Analog_Channel_RawScale_Multi = 122;
static constexpr uint16_t Analog_Channel_RawScale_Divis = 100;

// Before clipping and scaling some sanity check is needed with bigger ranges
static constexpr uint16_t Analog_Channel_RawSanityRange_Min = 150;
static constexpr uint16_t Analog_Channel_RawSanityRange_Max = 1950;

static constexpr uint8_t ANALOG_CHANNEL_COUNT = 16;

static constexpr uint16_t RAW_FRAME_SIZE = 25;
using FrameData = std::array<uint8_t, RAW_FRAME_SIZE>;

} // namespace remote_control_device::SBUS::Protocol