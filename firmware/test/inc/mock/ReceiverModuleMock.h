#pragma once
#include "gmock/gmock.h"
#include <PeripheralDrivers/ReceiverModule.hpp>
#include "mock/HALMock.hpp"
#include "mock/LoggingMock.hpp"
#include <stm32f3xx_hal_uart.h>
#include <stm32f3xx_hal_tim.h>

using namespace remote_control_device;

class ReceiverModuleMock : public ReceiverModule
{
public:
    ReceiverModuleMock(HALMock& hal, LoggingMock& log) : ReceiverModule(uart, tim, hal, log) {}
    MOCK_METHOD(void, dispatch, (uint32_t flags), (override));
    MOCK_METHOD(bool, getSBUSFrame, (SBUS::Frame &frame), (override));
private:
    UART_HandleTypeDef uart;
    TIM_HandleTypeDef tim;
};