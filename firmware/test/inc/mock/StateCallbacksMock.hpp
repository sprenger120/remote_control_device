#pragma once
#include "gmock/gmock.h"
#include <Statemachine/State.hpp>

using namespace remote_control_device;

namespace remote_control_device
{
class StateChaningSources;
}

class StateCallbacksMock : public StateCallbacks
{
public:
    StateCallbacksMock()
        : StateCallbacks([](StateChaningSources &) -> void {},
                         [](StateChaningSources &) -> bool { return true; },
                         [](StateChaningSources &) -> void {})
    {
    }
    MOCK_METHOD(void, process, (StateChaningSources &), (override, const));
    MOCK_METHOD(bool, checkConditions, (StateChaningSources &), (override, const));
    MOCK_METHOD(void, oneTimeSetup, (StateChaningSources &), (override, const));
};