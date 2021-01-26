#include <FreeRTOS.h>
#include <stm32f3xx_hal.h>


extern "C" uint32_t HAL_GetTick() {
    return 0;
}