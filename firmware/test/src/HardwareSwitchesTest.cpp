#include "Statemachine/HardwareSwitches.hpp"
#include "gtest/gtest.h"
#include <exception>
#include <hippomocks.h>
#include <stm32f3xx_hal.h>
#include <Statemachine/StateSources.hpp>

using namespace remote_control_device;

TEST(HardwareSwitchesTest, update)
{
    MockRepository mocks;
    HardwareSwitchesState state;
    HardwareSwitches hws;

    mocks.ExpectCallFunc(HAL_GPIO_ReadPin).Return(GPIO_PIN_RESET);
    mocks.ExpectCallFunc(HAL_GPIO_ReadPin).Return(GPIO_PIN_SET);
    hws.update(state);

    EXPECT_FALSE(state.ManualSwitch);
    EXPECT_TRUE(state.BikeEmergency);

    mocks.ExpectCallFunc(HAL_GPIO_ReadPin).Return(GPIO_PIN_SET);    
    mocks.ExpectCallFunc(HAL_GPIO_ReadPin).Return(GPIO_PIN_RESET);
    hws.update(state);

    EXPECT_TRUE(state.ManualSwitch);
    EXPECT_FALSE(state.BikeEmergency);
}