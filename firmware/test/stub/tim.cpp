#include <stm32f3xx_hal.h>
#include <stm32f3xx_hal_tim.h>

HAL_StatusTypeDef HAL_TIM_RegisterCallback(TIM_HandleTypeDef *htim,
                                           HAL_TIM_CallbackIDTypeDef CallbackID,
                                           pTIM_CallbackTypeDef pCallback)
{
    return HAL_OK;
}