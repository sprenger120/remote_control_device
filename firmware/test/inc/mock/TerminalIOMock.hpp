#pragma once
#include "mock/HALMock.hpp"
#include "gmock/gmock.h"
#include <PeripheralDrivers/TerminalIO.hpp>
#include <SpanCompatibility.hpp>
#include <stm32f3xx_hal_uart.h>

using namespace remote_control_device;

class TerminalIOMock : public TerminalIO
{
public:
    explicit TerminalIOMock(HALMock& hal) : TerminalIO(hal, uart)
    {
    }
    MOCK_METHOD(void, dispatch, (uint32_t), (override));
    MOCK_METHOD(void, write, (const char *), (override));

    // small hack so we can continue using StrEq matcher even when spans are used
    void write(std::span<const char> str) override {
        if (strlen(str.data()) != str.size()) {
            throw std::runtime_error("span's size not valid");
        }
        write(str.data());
    }
private:
    UART_HandleTypeDef uart;
};