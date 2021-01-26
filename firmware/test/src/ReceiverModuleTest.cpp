#include "PeripheralDrivers/ReceiverModule.hpp"
#include "mock/LoggingMock.hpp"
#include "gtest/gtest.h"
#include <FreeRTOS.h>
#include <array>
#include <exception>
#include <hippomocks.h>
#include <stm32f3xx_hal.h>
#include <stm32f3xx_hal_uart.h>

using namespace remote_control_device;
using ::testing::Return;

class ReceiverModuleTest : public ::testing::Test
{
protected:
    ReceiverModuleTest() : term(hal), log(term, hal), recv(uart, tim, hal, log)
    {
    }

    HALMock hal;
    TerminalIOMock term;
    LoggingMock log;
    UART_HandleTypeDef uart;
    TIM_HandleTypeDef tim;
    ReceiverModule recv;
};

TEST_F(ReceiverModuleTest, notifyError_RestartRX)
{
    MockRepository mocks;

    mocks.ExpectCallFunc(HAL_UART_AbortReceive).Return(HAL_OK);
    mocks.ExpectCallFunc(HAL_UART_Receive_DMA).Return(HAL_OK);
    recv.dispatch(ReceiverModule::NOTIFY_ERROR);
}

TEST_F(ReceiverModuleTest, notifyStartRX_HALError)
{
    MockRepository mocks;

    mocks.ExpectCallFunc(HAL_UART_Receive_DMA).Return(HAL_ERROR);
    mocks.ExpectCallFunc(ReceiverModule::testing_RxRestart);
    recv.dispatch(ReceiverModule::NOTIFY_RX_START);
}

TEST_F(ReceiverModuleTest, notifyRxSucessful_SuccessfulDecode)
{
    MockRepository mocks;

    static constexpr uint32_t Magic_Time = 420;

    // successful decode
    mocks.ExpectCallFunc(SBUS::Decoder::decode).Return(std::make_pair(SBUS::DecodeError::NoError, SBUS::Frame()));
    mocks.ExpectCallFunc(ReceiverModule::testing_SuccessfulDecode);
    EXPECT_CALL(hal, GetTick).WillRepeatedly(Return(Magic_Time));
    mocks.NeverCallFunc(ReceiverModule::testing_ErrorDecode);

    // check if reception is started again after processing
    mocks.ExpectCallFunc(HAL_UART_Receive_DMA).Return(HAL_OK);
    recv.dispatch(ReceiverModule::NOTIFY_RX_SUCCESSFUL);

    // retrieve decoded frame and check magic time
    SBUS::Frame myframe;
    recv.getSBUSFrame(myframe);
    ASSERT_EQ(myframe.lastUpdate, Magic_Time);
}

TEST_F(ReceiverModuleTest, notifyRxSucessful_ErrorDecode)
{
    MockRepository mocks;

    // unsuccessful decode
    mocks.ExpectCallFunc(SBUS::Decoder::decode).Return(std::make_pair(SBUS::DecodeError::IllegalFlagByte, SBUS::Frame()));
    mocks.NeverCallFunc(ReceiverModule::testing_SuccessfulDecode);
    mocks.ExpectCallFunc(ReceiverModule::testing_ErrorDecode);

    // check if reception is started again after processing
    mocks.ExpectCallFunc(HAL_UART_Receive_DMA).Return(HAL_OK);
    recv.dispatch(ReceiverModule::NOTIFY_RX_SUCCESSFUL);
}