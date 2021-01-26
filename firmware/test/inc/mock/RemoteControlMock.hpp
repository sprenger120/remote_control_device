#pragma once
#include "gmock/gmock.h"
#include <Statemachine/RemoteControl.hpp>
#include "mock/HALMock.hpp"
#include "mock/LoggingMock.hpp"
#include "mock/ReceiverModuleMock.h"

using namespace remote_control_device;

class RemoteControlMock : public RemoteControl
{
public:
    RemoteControlMock(HALMock& hal, LoggingMock& log, ReceiverModuleMock& recv) : RemoteControl(hal, log, recv) {}
    MOCK_METHOD(void, update, (RemoteControlState &), (override, const));
    MOCK_METHOD(void, _update, (const SBUS::Frame &, RemoteControlState &), (override, const));
};