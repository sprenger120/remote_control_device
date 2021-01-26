#pragma once
#include "gmock/gmock.h"
#include <Wrapper/HAL.hpp>

using namespace wrapper;

class HALMock : public HAL
{
public:
    MOCK_METHOD(uint32_t, GetTick, (), (override, const));
};