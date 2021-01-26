/*
* Code is mostly taken from freertos posix_gcc example main.c
*/
#include "FreeRTOS.h"
#include "task.h"
extern "C" {

void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask,
									char *pcTaskName );
void vApplicationTickHook( void );
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
									StackType_t **ppxIdleTaskStackBuffer,
									uint32_t *pulIdleTaskStackSize );
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
									 StackType_t **ppxTimerTaskStackBuffer,
									 uint32_t *pulTimerTaskStackSize );

void vApplicationDaemonTaskStartupHook( void );
};