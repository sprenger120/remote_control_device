#pragma once
#include "SBUSProtocol.hpp"
#include <FreeRTOS.h>
#include <array>
#include <utility>

/**
 * @brief Decodes S.BUS frames sent from the XM+ receiver module.
 * S.BUS is a prorietary protocol initially developed by Futaba
 * but they since have given up the legal fight against people
 * reverse engineering it.
 * Protocol description taken from here
 * https://github.com/uzh-rpg/rpg_quadrotor_control/wiki/SBUS-Protocol
 *
 */
namespace remote_control_device::SBUS
{

struct Frame
{
    std::array<uint16_t, Protocol::ANALOG_CHANNEL_COUNT> analogChannels;
    bool digitalCh17;
    bool digitalCh18;
    bool frameLost;
    bool failsafe;
    // Not set by decode(), for users convenience
    uint32_t lastUpdate;

    Frame();
};

enum class DecodeError : uint8_t {
    NoError,
    StartOrEndbyte,
    IllegalFlagByte,
    AnalogChannelIndexOOB,
    AnalogChannelSanityCheckRange
};

class Decoder
{
    Decoder() = default;
    ~Decoder() = default;

public:
    Decoder(const Decoder &) = delete;
    Decoder(Decoder &&) = delete;
    Decoder &operator=(const Decoder &) = delete;
    Decoder &operator=(Decoder &&) = delete;

    /**
     * @brief Value analog channels will be set to when decoded frame signals loss of radio link
     *
     */
    static constexpr uint16_t ON_FAILSAFE_ANALOG_CHANNEL_VALUE = 0;
    static constexpr bool ON_FAILSAFE_DIGITAL_CHANNEL_VALUE = false;

    /**
     * @brief Range of analog channel's values.
     * Don't change without also changing scalar values in decoder
     *
     */
    static constexpr uint16_t ANALOG_CHANNEL_MIN = 0;
    static constexpr uint16_t ANALOG_CHANNEL_MAX = 2000;

    /**
     * @brief Decodes an sbus frame
     * On errro will return a zeroed out frame
     * 
     * @param data raw data 
     * @return std::pair<DecodeErrors, Frame> 
     */
    static std::pair<DecodeError, Frame> decode(const Protocol::FrameData &data);

    /**
     * @brief Scales raw values from FrameData to ANALOG_CHANNEL_MIN - ANALOG_CHANNEL_MAX
     * 
     * @param input 
     * @return uint16_t 
     */
    static uint16_t scaleRawValue(uint16_t input);
};

} // namespace remote_control_device::SBUS