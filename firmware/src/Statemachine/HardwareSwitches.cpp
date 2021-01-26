#include "HardwareSwitches.hpp"
#include <main.h>
#include <stm32f3xx_hal.h>
#include "StateSources.hpp"

namespace remote_control_device
{

void HardwareSwitches::update(HardwareSwitchesState &state) const
{
    // Hardware pull-ups / debouncing provided
    // GPIO_PIN_SET  voltage on pin 
    // GPIO_PIN_RESET no voltage - pin is grounded
    state.ManualSwitch = HAL_GPIO_ReadPin(hardwareSwitch1_GPIO_Port, hardwareSwitch1_Pin) == GPIO_PinState::GPIO_PIN_SET;
    state.BikeEmergency = HAL_GPIO_ReadPin(hardwareSwitch2_GPIO_Port, hardwareSwitch2_Pin) == GPIO_PinState::GPIO_PIN_SET;
}

} // namespace remote_control_device