#include "fake/Task.hpp"
#include "gtest/gtest.h"
#include <FreeRTOS.h>
#include <task.h>

int returnValue = -1;

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

	// run tests in freertos task context to avoid 
	// segmentation faults when semaphores are used

    TaskHandle_t hTask;
    xTaskCreate(
        [](void *) -> void {
            returnValue = RUN_ALL_TESTS();
            vTaskEndScheduler();
        },
        "", 1024*8, nullptr, 1, &hTask);

    vTaskStartScheduler();

    vTaskDelete(hTask);

    return returnValue;
}
