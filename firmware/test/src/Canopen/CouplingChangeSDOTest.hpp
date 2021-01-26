#pragma once
#include "CanopenTestFixture.hpp"
#include <asm/byteorder.h>

// as long as you remain within one byte bit fields can be used
// as long as you provide an ordering for big and little endian
// for multi byte stuff, good luck...
// https://stackoverflow.com/questions/1490092/c-c-force-bit-field-order-and-alignment

// not portable, requires gcc and linux
union SDORequestFlagByte
{
    uint8_t raw;
    struct __attribute__((packed))
    {
#if defined(__LITTLE_ENDIAN_BITFIELD)
        uint8_t sizeIndicator : 1; // least significant bit (0)
        uint8_t transferType : 1;
        uint8_t nonDataBytesCount : 2;
        uint8_t reservedAlwaysZero : 1;
        uint8_t clientCommandSpecifier : 3; // most significant (7)
#elif defined(__BIG_ENDIAN_BITFIELD)
        uint8_t clientCommandSpecifier : 3; // least significant bit (0)
        uint8_t reservedAlwaysZero : 1;
        uint8_t nonDataBytesCount : 2;
        uint8_t transferType : 1;
        uint8_t sizeIndicator : 1;      // most significant (7)
#else
#error "Please fix <asm/byteorder.h>"
#endif
    } flags;
};

union SDOResponseFlagByte
{
    uint8_t raw;
    struct __attribute__((packed))
    {
#if defined(__LITTLE_ENDIAN_BITFIELD)
        uint8_t reservedAlwaysZero : 5; // least significant bit (0)
        uint8_t commandSpecifier : 3;   // most significant (7)
#elif defined(__BIG_ENDIAN_BITFIELD)
        uint8_t commandSpecifier : 3;   // most significant (7)
        uint8_t reservedAlwaysZero : 5; // least significant bit (0)
#else
#error "Please fix <asm/byteorder.h>"
#endif
    } flags;
};

/**
 * @brief Checks if msgs only two messages are correct expedited sdo download requests
 *
 * @param msgs
 * @param brakeCouplingTarget
 * @param steeringCouplingTarget
 */
void assertSDORequest(std::vector<Message> &msgs, bool brakeCouplingTarget,
                      bool steeringCouplingTarget);

/**
 * @brief Constructs a sdo download response
 *
 * @param index
 * @param subIndex
 * @param dev
 * @return Message
 */
Message constructSDOResponse(uint16_t index, uint8_t subIndex, Canopen::BusDevices dev);

/**
 * @brief Checks,verifies,removes one brake and one steering abort request, will fail if both
 * weren't found
 *
 * @param msgs
 */
void assertSDOAbortTransfer(std::vector<Message> &msgs);