#include "SBUSDecoder.hpp"

namespace remote_control_device::SBUS
{

std::pair<DecodeError, Frame> Decoder::decode(const Protocol::FrameData &data)
{
    // check start, endbyte
    if (data[0] != Protocol::StartByte || data[Protocol::RAW_FRAME_SIZE - 1] != Protocol::EndByte)
    {
        return std::make_pair(DecodeError::StartOrEndbyte, Frame());
    }

    // check flag byte empty portion
    if ((data[Protocol::RAW_FRAME_SIZE - 2] & Protocol::Mask_FlagByte_Empty) != 0)
    {
        return std::make_pair(DecodeError::IllegalFlagByte, Frame());
    }

    Frame target;

    target.digitalCh17 = (data[Protocol::RAW_FRAME_SIZE - 2] & Protocol::Mask_FlagByte_Ch17) > 0;
    target.digitalCh18 = (data[Protocol::RAW_FRAME_SIZE - 2] & Protocol::Mask_FlagByte_Ch18) > 0;
    target.frameLost = (data[Protocol::RAW_FRAME_SIZE - 2] & Protocol::Mask_FlagByte_FrameLost) > 0;
    target.failsafe = (data[Protocol::RAW_FRAME_SIZE - 2] & Protocol::Mask_FlagByte_Failsafe) > 0;

    if (target.failsafe)
    {
        // when failsafe is present these values depend on the remote control's
        // settings which can be dangerous as the task processing the decoded data
        // may be programmed for only one value
        target = Frame();
    }
    else
    {
        // extract analog channels
        uint8_t channel = 0;
        uint8_t bitsFilled = 0;
        for (uint8_t byteIndex = 1; byteIndex < Protocol::RAW_FRAME_SIZE - 2; byteIndex++)
        {
            for (uint8_t bitIndex = 0; bitIndex < 8; bitIndex++)
            {
                if (channel >= target.analogChannels.size())
                {
                    // someone tinkered with the frame size / channeld width resulting in too
                    // many channels
                    return std::make_pair(DecodeError::AnalogChannelIndexOOB, Frame());
                }
                uint8_t bit = (data[byteIndex] & (1 << bitIndex)) > 0 ? 1 : 0;
                target.analogChannels[channel] |= bit << Protocol::Analog_Channel_Bit_Width;
                target.analogChannels[channel] >>= 1;
                bitsFilled++;
                if (bitsFilled == Protocol::Analog_Channel_Bit_Width)
                {
                    channel++;
                    bitsFilled = 0;
                }
            }
        }

        // check value ranges
        for (const uint16_t &val : target.analogChannels)
        {
            if (val < Protocol::Analog_Channel_RawSanityRange_Min ||
                val > Protocol::Analog_Channel_RawSanityRange_Max)
            {
                return std::make_pair(DecodeError::AnalogChannelSanityCheckRange, Frame());
            }
        }

        // clip, scale
        for (uint16_t &val : target.analogChannels)
        {
            val = scaleRawValue(val);
        }
    }

    return std::make_pair(DecodeError::NoError, target);
}

uint16_t Decoder::scaleRawValue(uint16_t val)
{
    val = std::min(std::max(val, Protocol::Analog_Channel_RawQX7Range_Min),
                   Protocol::Analog_Channel_RawQX7Range_Max);

    uint32_t temp = val;
    temp -= Protocol::Analog_Channel_RawQX7Range_Min;
    temp *= Protocol::Analog_Channel_RawScale_Multi;
    temp /= Protocol::Analog_Channel_RawScale_Divis;
    return static_cast<uint16_t>(temp);
}

Frame::Frame()
{
    for (uint16_t &val : analogChannels)
    {
        val = Decoder::ON_FAILSAFE_ANALOG_CHANNEL_VALUE;
    }
    digitalCh17 = Decoder::ON_FAILSAFE_DIGITAL_CHANNEL_VALUE;
    digitalCh18 = Decoder::ON_FAILSAFE_DIGITAL_CHANNEL_VALUE;
    failsafe = true;
    frameLost = true;
    lastUpdate = 0;
}

} // namespace remote_control_device::SBUS