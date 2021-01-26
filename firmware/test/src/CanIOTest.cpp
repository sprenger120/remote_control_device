#include "PeripheralDrivers/CanIO.hpp"
#include "gtest/gtest.h"
#include "mock/LoggingMock.hpp"
#include "mock/CanopenMock.hpp"
#include <FreeRTOS.h>
#include <array>
#include <exception>
#include <hippomocks.h>
#include <stm32f3xx_hal.h>
#include <stm32f3xx_hal_can.h>

extern "C"
{
#include <canfestival/canfestival.h>
}

using namespace remote_control_device;

class CanIOTest : public ::testing::Test
{
protected:
    CanIOTest() :term(halMock), log(term, halMock), canio(can, log), co(canio, log)
    {
    }

    void SetUp() override {
        canio.setCanopenInstance(co);
    }

    HALMock halMock;
    TerminalIOMock term;
    LoggingMock log;
    CAN_HandleTypeDef can;
    CanIO canio;
    CanopenMock co;
};

TEST_F(CanIOTest, nothingToTransmit)
{
    MockRepository mocks;

    // test with empty queue, no tx attempt should be made
    mocks.ExpectCallFunc(HAL_CAN_GetTxMailboxesFreeLevel).Return(3);
    mocks.NeverCallFunc(HAL_CAN_AddTxMessage);
    canio.dispatch(CanIO::NOTIFY_ATTEMPT_TX);
}

TEST_F(CanIOTest, allMailboxesFull)
{
    MockRepository mocks;

    Message m = Message_Initializer;

    mocks.ExpectCallFunc(HAL_CAN_GetTxMailboxesFreeLevel).Return(0);
    mocks.NeverCallFunc(HAL_CAN_AddTxMessage);
    canio.canSend(&m);
    canio.dispatch(CanIO::NOTIFY_ATTEMPT_TX);
}

TEST_F(CanIOTest, nothingToRX)
{
    MockRepository mocks;

    mocks.NeverCallFunc(canDispatch);
    canio.dispatch(CanIO::NOTIFY_RX_PENDING);
}

TEST_F(CanIOTest, busOK)
{
    MockRepository mocks;

    canio.dispatch(CanIO::NOTIFY_ERROR);
    ASSERT_FALSE(canio.isBusOK());
    canio.dispatch(CanIO::NOTIFY_RX_PENDING);
    ASSERT_TRUE(canio.isBusOK());
}

TEST_F(CanIOTest, canSend)
{
    MockRepository mocks;

    // setup test message
    static constexpr UNS16 cob_id = 420;
    static constexpr UNS8 rtr = 0;
    static constexpr UNS8 len = 7;
    static constexpr UNS8 data[8] = {1, 2, 3, 4, 5, 6, 7, 0};

    Message m = Message_Initializer;
    m.cob_id = cob_id;
    m.rtr = rtr;
    m.len = len;
    for (size_t i = 0; i < 8; ++i)
    {
        m.data[i] = data[i];
    }

    mocks.ExpectCallFunc(HAL_CAN_GetTxMailboxesFreeLevel).Return(3);
    mocks.NeverCallFunc(CanIO::testing_TxFailed);
    mocks.ExpectCallFunc(HAL_CAN_AddTxMessage)
        .Do([](CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *pHeader, uint8_t aData[],
               uint32_t *pTxMailbox) -> HAL_StatusTypeDef {
            // ASSERT is not available here so hack something together with throw
            for (int i = 0; i < 8; ++i)
            {
                if (aData[i] != data[i])
                {
                    throw std::runtime_error("data");
                }
            }
            if (pHeader->StdId != cob_id)
            {
                throw std::runtime_error("cobid");
            }
            if (pHeader->IDE != CAN_ID_STD)
            {
                throw std::runtime_error("canid type");
            }
            if (pHeader->RTR == CAN_RTR_REMOTE && rtr != 1)
            {
                throw std::runtime_error("rtr 1");
            }
            if (pHeader->RTR == CAN_RTR_DATA && rtr != 0)
            {
                throw std::runtime_error("rtr 0");
            }
            if (pHeader->DLC != len)
            {
                throw std::runtime_error("len");
            }
            return HAL_OK;
        });
    canio.canSend(&m);
    canio.dispatch(CanIO::NOTIFY_ATTEMPT_TX);

    // check if message is dequeued
    mocks.ExpectCallFunc(HAL_CAN_GetTxMailboxesFreeLevel).Return(3);
    mocks.NeverCallFunc(HAL_CAN_AddTxMessage);
    mocks.NeverCallFunc(CanIO::testing_TxFailed);
    canio.dispatch(CanIO::NOTIFY_ATTEMPT_TX);
}

TEST_F(CanIOTest, canDispatch_RejectExtendedId)
{
    MockRepository mocks;

    // setup test message
    static constexpr UNS16 cob_id = 420;
    static constexpr UNS8 rtr = 0;
    static constexpr UNS8 len = 7;
    static constexpr UNS8 data[8] = {1, 2, 3, 4, 5, 6, 7, 0};

    // test extended can id rejection
    mocks.ExpectCallFunc(HAL_CAN_GetRxMessage)
        .Do([](CAN_HandleTypeDef *hcan, uint32_t RxFifo, CAN_RxHeaderTypeDef *pHeader,
               uint8_t aData[]) -> HAL_StatusTypeDef {
            pHeader->IDE = CAN_ID_EXT;
            pHeader->ExtId = 0x8ff;
            pHeader->RTR = CAN_RTR_REMOTE;
            pHeader->DLC = 8;
            return HAL_OK;
        });
    mocks.NeverCallFunc(xQueueGenericSend);
    CanIO::retrieveMessageFromISR(canio, nullptr, 0);
}

TEST_F(CanIOTest, canDispatch)
{
    MockRepository mocks;

    // setup test message
    static constexpr UNS16 cob_id = 420;
    static constexpr UNS8 rtr = 0;
    static constexpr UNS8 len = 7;
    static constexpr UNS8 data[8] = {1, 2, 3, 4, 5, 6, 7, 0};

    // test queuing of correct header
    mocks.ExpectCallFunc(HAL_CAN_GetRxMessage)
        .Do([](CAN_HandleTypeDef *hcan, uint32_t RxFifo, CAN_RxHeaderTypeDef *pHeader,
               uint8_t aData[]) -> HAL_StatusTypeDef {
            pHeader->IDE = CAN_ID_STD;
            pHeader->StdId = cob_id;
            pHeader->RTR = rtr == 1 ? CAN_RTR_REMOTE : CAN_RTR_DATA;
            pHeader->DLC = len;
            for (size_t i = 0; i < 8; ++i)
            {
                aData[i] = data[i];
            }
            return HAL_OK;
        });
    CanIO::retrieveMessageFromISR(canio, nullptr, 0);

    mocks.ExpectCallFunc(canDispatch).Do([](CO_Data *d, Message *m) -> void {
        for (int i = 0; i < 8; ++i)
        {
            if (m->data[i] != data[i])
            {
                throw std::runtime_error("data");
            }
        }
        if (m->cob_id != cob_id)
        {
            throw std::runtime_error("cobid");
        }
        if (m->rtr != rtr)
        {
            throw std::runtime_error("rtr 1");
        }
        if (m->len != len)
        {
            throw std::runtime_error("len");
        }
    });
    canio.dispatch(CanIO::NOTIFY_RX_PENDING);

    // check if message is dequeued
    mocks.NeverCallFunc(canDispatch);
    canio.dispatch(CanIO::NOTIFY_RX_PENDING);
}

TEST_F(CanIOTest, transmit_HALError)
{
    MockRepository mocks;

    Message m = Message_Initializer;

    mocks.ExpectCallFunc(HAL_CAN_GetTxMailboxesFreeLevel).Return(3);
    mocks.ExpectCallFunc(HAL_CAN_AddTxMessage).Return(HAL_ERROR);
    mocks.ExpectCallFunc(CanIO::testing_TxFailed);
    canio.canSend(&m);
    canio.dispatch(CanIO::NOTIFY_ATTEMPT_TX);
}

TEST_F(CanIOTest, dispatch_MultipleFlags)
{
    MockRepository mocks;

    mocks.ExpectCallFunc(HAL_CAN_GetTxMailboxesFreeLevel).Return(3);
    mocks.ExpectCallFunc(CanIO::testing_RxAttempt);
    mocks.ExpectCallFunc(CanIO::testing_Error);
    mocks.ExpectCallFunc(CanIO::testing_Overload);
    canio.dispatch(CanIO::NOTIFY_ATTEMPT_TX | CanIO::NOTIFY_OVERLOAD | CanIO::NOTIFY_ERROR |
                   CanIO::NOTIFY_RX_PENDING);
}