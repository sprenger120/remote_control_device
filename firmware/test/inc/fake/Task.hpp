#pragma once
#include <FreeRTOS.h>
#include <stdexcept>
#include <task.h>

class FakeTaskContainer
{
public:
    FakeTaskContainer();
    virtual ~FakeTaskContainer();

    virtual TaskHandle_t get() const final;

private:
    TaskHandle_t handle{nullptr};

    static void taskMain(void *context);
};