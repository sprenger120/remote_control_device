#include "Task.hpp"
#include "SpecialAssert.hpp"
#include <algorithm>
#include "BuildConfiguration.hpp"

namespace wrapper
{
size_t Task::_taskListIndex{0};
std::array<TaskHandle_t, Task::MAX_TASKS> Task::_taskList{};

Task::Task(TaskFunction_t taskCode, const char *name, uint16_t stackDepth, void *parameter,
           UBaseType_t priority, EventBits_t readyFlag)
    : _taskCode(taskCode), _parameter(parameter), _readyFlag(readyFlag)
{
    specialAssert(taskCode != nullptr);
    xTaskCreate(&Task::taskMain, name, stackDepth, reinterpret_cast<void *>(this), priority,
                &_handle);
    specialAssert(_handle != nullptr);

    registerTask(_handle);
}

void Task::registerTask(TaskHandle_t handle)
{
#ifdef BUILDCONFIG_EMBEDDED_BUILD
    if (_taskListIndex == 0)
    {
        std::fill(_taskList.begin(), _taskList.end(), nullptr);
    }
    specialAssert(_taskListIndex < _taskList.size());
    _taskList.at(_taskListIndex++) = handle;
#endif
}

BaseType_t Task::notifyWait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
                            uint32_t *pulNotificationValue, TickType_t xTicksToWait)
{
    return xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue,
                           xTicksToWait);
}

BaseType_t Task::notify(uint32_t ulValue, eNotifyAction eAction)
{
    return xTaskNotify(_handle, ulValue, eAction);
}

BaseType_t Task::notifyFromISR(uint32_t ulValue, eNotifyAction eAction,
                               BaseType_t *pxHigherPriorityTaskWoken)
{

    return xTaskNotifyFromISR(_handle, ulValue, eAction, pxHigherPriorityTaskWoken);
}

Task::~Task()
{
    if (_handle != nullptr)
    {
        vTaskDelete(_handle);
    }
}

void Task::taskMain(void *instance)
{
    Task *task = reinterpret_cast<Task *>(instance);
    sync::waitForOne(sync::Application_Ready);
    task->_taskCode(task->_parameter);
    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

} // namespace wrapper