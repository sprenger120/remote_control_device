#pragma once
#include "gmock/gmock.h"
#include <Statemachine/HardwareSwitches.hpp>

using namespace remote_control_device;

class HardwareSwitchesMock : public HardwareSwitches
{
public:
    MOCK_METHOD(void, update, (HardwareSwitchesState &), (override, const));
};