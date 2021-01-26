#include "Sync.hpp"
#include <FreeRTOS.h>
#include <event_groups.h>

namespace
{
EventGroupHandle_t syncEventGroup = xEventGroupCreate();
} // namespace

namespace wrapper::sync
{
void waitForOne(EventBits_t events)
{
    (void)xEventGroupWaitBits(syncEventGroup, events, pdFALSE, pdFALSE, portMAX_DELAY);
}

void waitForEveryone()
{
    EventBits_t events = CanIO_Ready | ReceiverModule_Ready | TerminalIO_Ready |
                         Statemachine_Ready | CanfestivalTimers_Ready;

    (void)xEventGroupWaitBits(syncEventGroup, events, pdFALSE, pdTRUE, portMAX_DELAY);
}

void signal(EventBits_t events)
{
    (void)xEventGroupSetBits(syncEventGroup, events);
}
} // namespace wrapper::sync
