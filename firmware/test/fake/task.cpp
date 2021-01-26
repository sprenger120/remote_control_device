#include <FreeRTOS.h>
#include <fake/Task.hpp>
#include <stdexcept>
#include <task.h>
#include <iostream>

FakeTaskContainer::FakeTaskContainer()
{
    xTaskCreate(&FakeTaskContainer::taskMain, "myname", 256, nullptr, 1, &handle);
    if (handle == nullptr)
    {
        throw std::runtime_error("Couldn't create task");
    }
}

FakeTaskContainer::~FakeTaskContainer()
{
    vTaskDelete(handle);
}

TaskHandle_t FakeTaskContainer::get() const
{
    if (handle == nullptr)
    {
        throw std::runtime_error("Taskhandle is null");
    }
    return handle;
}

void FakeTaskContainer::taskMain(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
    }
}