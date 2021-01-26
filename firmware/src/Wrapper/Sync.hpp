#pragma once

#include <FreeRTOS.h>
#include <event_groups.h>

namespace wrapper::sync
{
constexpr EventBits_t CanIO_Ready = 1 << 0;
constexpr EventBits_t ReceiverModule_Ready = 1 << 1;
constexpr EventBits_t TerminalIO_Ready = 1 << 2;
constexpr EventBits_t Statemachine_Ready = 1 << 3;
constexpr EventBits_t CanfestivalTimers_Ready = 1 << 4;


constexpr EventBits_t Application_Ready = 1 << 5;

void waitForOne(EventBits_t events);
void waitForEveryone();

void signal(EventBits_t events);
} // namespace sync
