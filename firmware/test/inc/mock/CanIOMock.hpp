#pragma once
#include "gmock/gmock.h"
#include <PeripheralDrivers/CanIO.hpp>
#include <task.h>
#include <CanFestival/CanFestivalLocker.hpp>
#include "mock/LoggingMock.hpp"
#include <Statemachine/Canopen.hpp>

extern "C"
{
#include <canfestival/can.h>
}

using namespace remote_control_device;

class CanIOMock : public CanIO
{
public:
    CanIOMock(wrapper::HAL &hal, Logging &log) : CanIO(can, log)
    {
    }
    MOCK_METHOD(void, setCanopenInstance, (Canopen&), (override));
    MOCK_METHOD(void, dispatch, (uint32_t), (override));
    MOCK_METHOD(bool, isBusOK, (), (override));
    MOCK_METHOD(void, canSend, (Message *), (override));

    virtual void addRXMessage(Message & msg) override final {
        CFLocker lock;
        canDispatch(lock.getOD(),  &msg);
    }
private:
    CAN_HandleTypeDef can;
};
