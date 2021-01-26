#include <canfestival/can.h>
#include <stm32f3xx_hal.h>
#include <stm32f3xx_hal_can.h>
#include <cmsis_os.h>


extern "C" HAL_StatusTypeDef HAL_CAN_RegisterCallback(CAN_HandleTypeDef *hcan,
                                                      HAL_CAN_CallbackIDTypeDef CallbackID,
                                                      void (*pCallback)(CAN_HandleTypeDef *_hcan))
{
    return HAL_OK;
}

extern "C" uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *hcan)
{
    return 0;
}

extern "C" HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan,
                                                  CAN_TxHeaderTypeDef *pHeader, uint8_t aData[],
                                                  uint32_t *pTxMailbox)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t RxFifo,
                                       CAN_RxHeaderTypeDef *pHeader, uint8_t aData[])
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, CAN_FilterTypeDef *sFilterConfig) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t ActiveITs){
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_DeactivateNotification(CAN_HandleTypeDef *hcan, uint32_t InactiveITs){
    return HAL_OK;
}