#pragma once
#include "Wrapper/Sync.hpp"
#include <FreeRTOS.h>
#include <limits>
#include <task.h>
#include <array>

namespace wrapper
{

class Task
{
public:
    Task(TaskFunction_t taskCode, const char *name, uint16_t stackDepth, void *parameter,
         UBaseType_t priority, EventBits_t readyFlag);
    ~Task();

    Task(const Task &) = delete;
    Task(Task &&) = delete;
    Task &operator=(const Task &) = delete;
    Task &operator=(Task &&) = delete;

    static constexpr uint32_t ClearAllBits = std::numeric_limits<uint32_t>::max();
    BaseType_t notifyWait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit,
                          uint32_t *pulNotificationValue, TickType_t xTicksToWait);

    BaseType_t notify(uint32_t ulValue, eNotifyAction eAction);

    BaseType_t notifyFromISR(uint32_t ulValue, eNotifyAction eAction,
                             BaseType_t *pxHigherPriorityTaskWoken);


    static constexpr uint8_t MAX_TASKS = 6;
    static std::array<TaskHandle_t, MAX_TASKS>& getAllTaskHandles() {
        return _taskList;
    }
    static void registerTask(TaskHandle_t);
private:
    TaskHandle_t _handle{nullptr};
    TaskFunction_t _taskCode;
    void *_parameter;
    EventBits_t _readyFlag;

    static void taskMain(void *);

    static std::array<TaskHandle_t, MAX_TASKS> _taskList;
    static size_t _taskListIndex;
};

} // namespace wrapper