#include "fake/Task.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <PeripheralDrivers/TerminalIO.hpp>
#include <cstring>
#include <hippomocks.h>
#include <iostream>
#include <stm32f3xx_hal.h>
#include "mock/HALMock.hpp"

using namespace remote_control_device;
using ::testing::Return;

class TerminalIOTest : public ::testing::Test
{
protected:
    TerminalIOTest() : term(hal, uart)
    {
    }

    void SetUp() override  {
        uart.gState = HAL_UART_STATE_READY;
    }

    UART_HandleTypeDef uart;
    HALMock hal;
    TerminalIO term;
};

TEST_F(TerminalIOTest, notifyError_RestartTX)
{
    MockRepository mocks;
    term.write("testDataSoTXIsActuallyStarted");

    mocks.NeverCallFunc(TerminalIO::testHook_ErrorHALTXStart);
    mocks.ExpectCallFunc(HAL_UART_Abort).Return(HAL_OK);
    mocks.ExpectCallFunc(HAL_UART_Transmit_DMA).Return(HAL_OK);
    term.dispatch(TerminalIO::NOTIFY_ERROR);
}

TEST_F(TerminalIOTest, notifyStartRXTX_HALError)
{

    MockRepository mocks;

    term.write("testDataSoTXIsActuallyStarted");

    mocks.ExpectCallFunc(HAL_UART_Transmit_DMA).Return(HAL_ERROR);
    mocks.ExpectCallFunc(TerminalIO::testHook_ErrorHALTXStart);
    term.dispatch(TerminalIO::NOTIFY_TX_START);
}

TEST_F(TerminalIOTest, retransmissionAttempt)
{
    MockRepository mocks;

    static constexpr char TestString[] = "testdata";
    static constexpr char TestString2[] = "NEWTESTDATA";
    // tests assume that testString is not chopped up in transmission
    ASSERT_GT(TerminalIO::TX_TRANSFER_BUFFER_SIZE, strlen(TestString));
    // no retransmission attemts will break the test
    ASSERT_GE(TerminalIO::MAX_TX_ATTEMPTS, 1);

    // teststrings must be different or else new data after TX attemts will not be
    // differentiable from the old data
    ASSERT_STRNE(TestString, TestString2);

    auto transmitCheckFunction = [](UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size) -> HAL_StatusTypeDef {
        auto data = reinterpret_cast<const char *>(pData);
        if (strncmp(data, TestString, Size) != 0)
        {
            throw std::runtime_error("WrongData");
        }
        if (strnlen(data, Size) != Size)
        {
            throw std::runtime_error("Incorrect size");
        }
        return HAL_OK;
    };

    // write some data, send on its way
    term.write(TestString);
    mocks.ExpectCallFunc(HAL_UART_Transmit_DMA).Do(transmitCheckFunction);
    term.dispatch(TerminalIO::NOTIFY_TX_START);

    // write some new data that should not be transmitted until retransmission attemts failed
    term.write(TestString2);

    // exhaust all retransmissions attemts
    for (int i = 0; i < TerminalIO::MAX_TX_ATTEMPTS; ++i)
    {
        mocks.ExpectCallFunc(HAL_UART_Transmit_DMA).Do(transmitCheckFunction);
        term.dispatch(TerminalIO::NOTIFY_ERROR);
    }

    // now the new data should be sent
    mocks.ExpectCallFunc(HAL_UART_Transmit_DMA)
        .Do([](UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size) -> HAL_StatusTypeDef {
            auto data = reinterpret_cast<const char *>(pData);
            if (strncmp(data, TestString2, Size) != 0)
            {
                throw std::runtime_error("Wrong Data 2");
            }
            if (strnlen(data, Size) != Size)
            {
                throw std::runtime_error("Incorrect size");
            }
            return HAL_OK;
        });
    term.dispatch(TerminalIO::NOTIFY_ERROR);
}

TEST_F(TerminalIOTest, successfulTx)
{
    MockRepository mocks;
    static constexpr char TestString[] = "testdata";
    static constexpr char TestString2[] = "NEWTESTDATA";
    // tests assume that testString is not chopped up in transmission
    ASSERT_GT(TerminalIO::TX_TRANSFER_BUFFER_SIZE, strlen(TestString));
    // no retransmission attemts will break the test
    ASSERT_GE(TerminalIO::MAX_TX_ATTEMPTS, 1);

    // teststrings must be different or else new data after TX attemts will not be
    // differentiable from the old data
    ASSERT_STRNE(TestString, TestString2);

    auto transmitCheckFunction = [](UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size) -> HAL_StatusTypeDef {
        auto data = reinterpret_cast<const char *>(pData);
        if (strncmp(data, TestString, Size) != 0)
        {
            throw std::runtime_error("WrongData");
        }
        if (strnlen(data, Size) != Size)
        {
            throw std::runtime_error("Incorrect size");
        }
        return HAL_OK;
    };

    // write some data, send on its way
    term.write(TestString);
    mocks.ExpectCallFunc(HAL_UART_Transmit_DMA).Do(transmitCheckFunction);
    term.dispatch(TerminalIO::NOTIFY_TX_START);

    // new data
    term.write(TestString2);

    term.signalTXSuccessFromISR();

    // now the new data should be sent
    mocks.ExpectCallFunc(HAL_UART_Transmit_DMA)
        .Do([](UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size) -> HAL_StatusTypeDef {
            auto data = reinterpret_cast<const char *>(pData);
            if (strncmp(data, TestString2, Size) != 0)
            {
                throw std::runtime_error("Wrong Data 2");
            }
            if (strnlen(data, Size) != Size)
            {
                throw std::runtime_error("Incorrect size");
            }
            return HAL_OK;
        });
    term.dispatch(TerminalIO::NOTIFY_TX_START);
}