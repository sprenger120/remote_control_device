#include "CanFestivalTimers.hpp"

#include "CanFestivalLocker.hpp"
#include "Logging.hpp"
#include <algorithm>
#include <cmsis_os2.h>
#include <limits>
#include <task.h>
#include "SpecialAssert.hpp"

namespace remote_control_device
{

CanFestivalTimers *CanFestivalTimers::_instance{nullptr};

CanFestivalTimers::CanFestivalTimers(wrapper::HAL &hal, Logging &log)
    : _hal(hal), _log(log)
{
    specialAssert(_instance == nullptr);
    _instance = this;
    
    for (auto &row : _timers)
    {
        row.state = TIMER_FREE;
        row.d = nullptr;
        row.callback = nullptr;
        row.id = 0;
        row.val = 0;
        row.interval = 0;
    }
}

CanFestivalTimers::~CanFestivalTimers()
{
    _instance = nullptr;
}

TIMER_HANDLE CanFestivalTimers::setAlarm(CO_Data *d, UNS32 id, TimerCallback_t callback,
                                         TIMEVAL value, TIMEVAL period)
{
    specialAssert(_instance != nullptr);
    if (d == nullptr || callback == nullptr)
    {
        testHook_ErrorSetAlarm();
        _instance->_log.logWarning(Logging::Origin::CFTimers,
                                   "setAlarm invalid OD or callback pointer");
        return TIMER_NONE;
    }

    // search for free slots
    CFLocker locker;
    for (TIMER_HANDLE i = 0; i < MAX_NB_TIMER; ++i)
    {
        if (_instance->_timers[i].state == TIMER_FREE)
        {
            auto &entry = _instance->_timers[i];
            entry.callback = callback;
            entry.d = d;
            entry.id = id;
            entry.val = value + MS_TO_TIMEVAL(static_cast<TIMEVAL>(_instance->_hal.GetTick()));
            entry.interval = period;
            entry.state = TIMER_ARMED;
            return i;
        }
    }

    _instance->_log.logError(Logging::Origin::CFTimers, "No timer slots available");

    // no timer free
    return TIMER_NONE;
}

uint8_t CanFestivalTimers::getTimersRemaining()
{
    CFLocker locker;
    uint8_t cnt = 0;
    for (const auto &e : _timers)
    {
        if (e.state == TIMER_FREE)
        {
            cnt++;
        }
    }
    return cnt;
}

TIMER_HANDLE CanFestivalTimers::delAlarm(TIMER_HANDLE handle)
{
    specialAssert(_instance != nullptr);
    if (handle >= static_cast<TIMER_HANDLE>(_instance->_timers.size()) || handle < TIMER_NONE)
    {
        testHook_ErrorDelAlarm();
        _instance->_log.logWarning(Logging::Origin::CFTimers, "delAlarm invalid timer handle");
        return TIMER_NONE;
    }

    if (handle != TIMER_NONE)
    {
        CFLocker locker;
        _instance->_timers[handle].state = TIMER_FREE;
    }
    return TIMER_NONE;
}

TickType_t CanFestivalTimers::dispatch()
{
    // dispatch registered timers
    TIMEVAL currTime{MS_TO_TIMEVAL(static_cast<TIMEVAL>(_hal.GetTick()))};
    TIMEVAL nearestTime{std::numeric_limits<TIMEVAL>::max()};
    static constexpr TickType_t NoTimersRegisteredWaittime = pdMS_TO_TICKS(50);

    CFLocker locker;

    // search nearest timer and calculate waittime
    // mark timers with elapsed time
    for (struct_s_timer_entry &entry : _timers)
    {
        if (entry.state == TIMER_ARMED)
        {
            if (entry.val <= currTime)
            {
                entry.state = TIMER_TRIG;
                nearestTime = 0;
            }
            else
            {
                nearestTime = std::min(nearestTime, entry.val);
            }
        }
    }

    if (nearestTime > 0)
    {
        // wait if nearest timer is some time away
        if (nearestTime == std::numeric_limits<TIMEVAL>::max())
        {
            // no timers registered, wait short time
            return NoTimersRegisteredWaittime;
        }
        else
        {
            return static_cast<TickType_t>(TIMEVAL_TO_MS(nearestTime - currTime));
        }
    }

    // trigger marked timers
    for (struct_s_timer_entry &entry : _timers)
    {
        if (entry.state == TIMER_TRIG)
        {
            if (entry.callback != nullptr)
            {
                (entry.callback)(entry.d, entry.id);
            }
            // when interval timer, schedule next call
            if (entry.interval > 0)
            {
                entry.val = entry.interval + currTime;
                entry.state = TIMER_ARMED;
            }
            else
            {
                entry.state = TIMER_FREE;
            }
        }
    }

    return 1;
}

void CanFestivalTimers::taskMain()
{
    for (;;)
    {
        const TickType_t ticksToWait = _instance->dispatch();
        vTaskDelay(pdMS_TO_TICKS(ticksToWait));
    }
}

} // namespace remote_control_device

using remote_control_device::CanFestivalTimers;
extern "C" TIMER_HANDLE SetAlarm(CO_Data *d, UNS32 id, TimerCallback_t callback, TIMEVAL value,
                                 TIMEVAL period)
{
    return CanFestivalTimers::setAlarm(d, id, callback, value, period);
}

extern "C" TIMER_HANDLE DelAlarm(TIMER_HANDLE handle)
{
    return CanFestivalTimers::delAlarm(handle);
}