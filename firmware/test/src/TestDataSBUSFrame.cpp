#include "TestDataSBUSFrame.hpp"

// Don't change any frameData unless you also change the constexpr values

namespace TestDataSBUSFrame
{

Protocol::FrameData GoodFrame::frameData = {
    // header byte
    0x0f, 

    // analog channels (generated by bitArray.py)
    0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000, 0b01000011, 0b00011111, 0b11111010,
    0b11010000, 0b10000111, 0b00111110, 0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000,
    0b01000011, 0b00011111, 0b11111010, 0b11010000, 0b10000111, 0b00111110,

    // flag byte
    0b00000011, 

    // footer / end byte
    0x00        
};

Protocol::FrameData GoodFrameTimeout::frameData = {
    // header byte
    0x0f, 

    // analog channels (generated by bitArray.py)
    0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000, 0b01000011, 0b00011111, 0b11111010,
    0b11010000, 0b10000111, 0b00111110, 0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000,
    0b01000011, 0b00011111, 0b11111010, 0b11010000, 0b10000111, 0b00111110,

    // flag byte
    0b00001011, 

    // footer / end byte
    0x00        
};

Protocol::FrameData BadFrameStartByte::frameData = {
    // header byte
    0x0e, 

    // analog channels (generated by bitArray.py)
    0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000, 0b01000011, 0b00011111, 0b11111010,
    0b11010000, 0b10000111, 0b00111110, 0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000,
    0b01000011, 0b00011111, 0b11111010, 0b11010000, 0b10000111, 0b00111110,

    // flag byte
    0b00000011, 

    // footer / end byte
    0x00        
};

Protocol::FrameData BadFrameEndByte::frameData = {
    // header byte
    0x0f, 

    // analog channels (generated by bitArray.py)
    0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000, 0b01000011, 0b00011111, 0b11111010,
    0b11010000, 0b10000111, 0b00111110, 0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000,
    0b01000011, 0b00011111, 0b11111010, 0b11010000, 0b10000111, 0b00111110,

    // flag byte
    0b00000011, 

    // footer / end byte
    0x01        
};

Protocol::FrameData BadFrameFlagBytes::frameData = {
    // header byte
    0x0f, 

    // analog channels (generated by bitArray.py)
    0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000, 0b01000011, 0b00011111, 0b11111010,
    0b11010000, 0b10000111, 0b00111110, 0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000,
    0b01000011, 0b00011111, 0b11111010, 0b11010000, 0b10000111, 0b00111110,

    // flag byte
    0b01000011, 

    // footer / end byte
    0x00        
};

Protocol::FrameData BadFrameChannelValue::frameData = {
    // header byte
    0x0f, 

    // analog channels (generated by bitArray.py)
    0b11110100, 0b10100001, 0b00001111, 0b01111111, 0b11111111, 0b11111111, 0b00011111, 0b11111010,
    0b11010000, 0b10000111, 0b00111110, 0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000,
    0b01000011, 0b00011111, 0b11111010, 0b11010000, 0b10000111, 0b00111110,

    // flag byte
    0b00000011, 

    // footer / end byte
    0x00        
};

Protocol::FrameData BadFrameChannelValue2::frameData = {
    // header byte
    0x0f, 

    // analog channels (generated by bitArray.py)
    0b11110100, 0b10100001, 0b00000000, 0b00000000, 0b11101000, 0b01000011, 0b00011111, 0b11111010,
    0b11010000, 0b10000111, 0b00111110, 0b11110100, 0b10100001, 0b00001111, 0b01111101, 0b11101000,
    0b01000011, 0b00011111, 0b11111010, 0b11010000, 0b10000111, 0b00111110,

    // flag byte
    0b00000011, 

    // footer / end byte
    0x00        
};

} // namespace TestDataSBUSFrame