#include "LEDs.hpp"
#include "SpecialAssert.hpp"
#include "gtest/gtest.h"
#include <array>
#include <hippomocks.h>
#include <iostream>
#include <stm32f3xx_hal.h>

using namespace remote_control_device;

class LEDTest : public ::testing::Test
{
protected:
    LEDTest() : led(nullptr, 0, false)
    {
    }

    LED led;
};


TEST(LEDTest2, initiallyOff)
{
    MockRepository mocks;

    // switch led off on initialisation
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
    LED led(nullptr, 0, false);
}

TEST_F(LEDTest, stayingOff)
{
 {
        MockRepository mocks;
        // led should stay off and timer shouldn't be rescheduled again
        mocks.NeverCallFunc(HAL_GPIO_WritePin);
        for (int i = 0; i < 100; ++i)
        {
            ASSERT_EQ(led.dispatch(), 0);
        }
    }

    {
        // same mode, nothing should be done
        MockRepository mocks;
        mocks.NeverCallFunc(HAL_GPIO_WritePin);
        led.setMode(LED::Mode::Off);
    }

    {
        MockRepository mocks;
        mocks.NeverCallFunc(HAL_GPIO_WritePin);
        for (int i = 0; i < 100; ++i)
        {
            ASSERT_EQ(led.dispatch(), 0);
        }
    }
}

TEST_F(LEDTest, stayingOn)
{
    MockRepository mocks;
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
    led.setMode(LED::Mode::On);

    // solid off should ask timer to not call dispatch again
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
    ASSERT_EQ(led.dispatch(), 0);
}

TEST_F(LEDTest, fastBlink)
{
    MockRepository mocks;
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
    mocks.ExpectCallFunc(HAL_GPIO_TogglePin);
    led.setMode(LED::Mode::FastBlink);

    for (int i = 0; i < 100; ++i)
    {
        mocks.ExpectCallFunc(HAL_GPIO_TogglePin);
        ASSERT_EQ(led.dispatch(), LED::FAST_BLINK_INTERVAL);
    }
}

TEST_F(LEDTest, slowBlink)
{
    MockRepository mocks;
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
    mocks.ExpectCallFunc(HAL_GPIO_TogglePin);
    led.setMode(LED::Mode::SlowBlink);

    for (int i = 0; i < 100; ++i)
    {
        mocks.ExpectCallFunc(HAL_GPIO_TogglePin);
        ASSERT_EQ(led.dispatch(), LED::SLOW_BLINK_INTERVAL);
    }
}

TEST_F(LEDTest, Counting3)
{
    MockRepository mocks;
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
    led.setMode(LED::Mode::Counting, 3); // on 1,  already does one dispatch()
    
    for (int i = 0; i < 100; ++i)
    {
        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_BLINK_INTERVAL); // off 1

        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_BLINK_INTERVAL); // on 2

        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_BLINK_INTERVAL); // off 2

        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_BLINK_INTERVAL); // on 3

        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_END_TIME); // off, long waittime

        // cycle restarts 
        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_BLINK_INTERVAL); // on 1
    }
}

TEST_F(LEDTest, Counting1)
{
    MockRepository mocks;
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
    mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
    led.setMode(LED::Mode::Counting, 1); // on 1,  already does one dispatch()
    
    for (int i = 0; i < 100; ++i)
    {
        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_END_TIME); // off 1

        // cycle restarts 
        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_SET);
        ASSERT_EQ(led.dispatch(), LED::COUNT_BLINK_INTERVAL); // on 1
    }
}



TEST_F(LEDTest, Counting0)
{
    {
        MockRepository mocks;
        mocks.ExpectCallFunc(HAL_GPIO_WritePin).With(_, _, GPIO_PIN_RESET);
        led.setMode(LED::Mode::Counting, 0);
    }

    {
        MockRepository mocks;
        // led should stay off and timer shouldn't be rescheduled again
        mocks.NeverCallFunc(HAL_GPIO_WritePin);
        for (int i = 0; i < 100; ++i)
        {
            ASSERT_EQ(led.dispatch(), 0);
        }
    }

}