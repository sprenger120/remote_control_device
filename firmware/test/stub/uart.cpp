#include <cmsis_os.h>
#include <stm32f3xx_hal.h>
#include <stm32f3xx_hal_uart.h>

HAL_StatusTypeDef HAL_UART_RegisterCallback(UART_HandleTypeDef *huart,
                                            HAL_UART_CallbackIDTypeDef CallbackID,
                                            pUART_CallbackTypeDef pCallback)
{
    return HAL_OK;
}

void HAL_UART_ReceiverTimeout_Config(UART_HandleTypeDef *huart, uint32_t TimeoutValue)
{
}

HAL_StatusTypeDef HAL_UART_EnableReceiverTimeout(UART_HandleTypeDef *huart)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef *huart)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *huart)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size,
                                   uint32_t Timeout)
{
    return HAL_OK;
}