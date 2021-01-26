#pragma once
#include <FreeRTOS.h>
#include <stm32f3xx_hal.h>

/**
 * @brief Encapsulates common hal functions into a class for mocking it with gmock
 * I've not realized gMocks clear advantages vs HippoMocks until I was almost done
 * with writing tests, so only some stuff will use this, one notable example being
 * CanFestivalTimers
 *
 */

namespace wrapper
{

class HAL
{
public:
    HAL() = default;
    virtual ~HAL() = default;

    HAL(const HAL &) = delete;
    HAL(HAL &&) = delete;
    HAL &operator=(const HAL &) = delete;
    HAL &operator=(HAL &&) = delete;

    virtual uint32_t GetTick() const
    {
        return HAL_GetTick();
    }
};

} // namespace wrapper