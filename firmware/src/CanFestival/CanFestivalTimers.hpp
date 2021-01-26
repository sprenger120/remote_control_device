#pragma once
#include "Wrapper/HAL.hpp"
#include <array>

extern "C"
{
#include <canfestival/config.h>
#include <canfestival/timer.h>
}

/**
 * @brief Executes canFestivals timed events.
 * On most targets this is done via hardware timers but this is problematic due to
 * the library not being thread safe.
 * Replaces the functionality of "timers.c" in the library.
 */

namespace remote_control_device
{
class Logging;

class CanFestivalTimers
{
public:
    /**
     * @brief Construct a new Can Festival Timers object
     *
     * @param hal HAL instance
     */
    CanFestivalTimers(wrapper::HAL &hal, Logging &log);
    virtual ~CanFestivalTimers();

    CanFestivalTimers(const CanFestivalTimers &) = delete;
    CanFestivalTimers(CanFestivalTimers &&) = delete;
    CanFestivalTimers &operator=(const CanFestivalTimers &) = delete;
    CanFestivalTimers &operator=(CanFestivalTimers &&) = delete;

    /**
     * @brief Registers am alarm to be triggered periodically or once in given time
     * Use MS_TO_TIMEVAL / US_TO_TIMEVAL macros to convert to ticks
     *
     * @param d object dictionary instance of node
     * @param id internal id of event
     * @param callback function to call
     * @param value time in ticks, when to trigger from time of calling
     * @param period time in ticks, period to use for continous calling, 0 for no periodic calling
     * @return TIMER_HANDLE handle to reference this timer
     */
    static TIMER_HANDLE setAlarm(CO_Data *d, UNS32 id, TimerCallback_t callback, TIMEVAL value,
                                 TIMEVAL period);

    /**
     * @brief Disables alarm
     *
     * @param handle alarm to remove
     * @return TIMER_HANDLE handle of removed alarm
     */
    static TIMER_HANDLE delAlarm(TIMER_HANDLE handle);

    /**
     * @brief Checks for alarms to dispatch
     *
     * @return time in ms until function should be called again
     */
    virtual  TickType_t dispatch();
    virtual void taskMain();

    static constexpr uint32_t MAX_TIMERS = MAX_NB_TIMER;

    /**
     * @brief Returns the remaining free timer slots
     *
     * @return uint8_t
     */
    virtual uint8_t getTimersRemaining();

    /* test hooks */
    static void testHook_ErrorDelAlarm(){};
    static void testHook_ErrorSetAlarm(){};

private:
    wrapper::HAL &_hal;
    Logging &_log;
    static CanFestivalTimers *_instance;

    std::array<struct_s_timer_entry, MAX_TIMERS> _timers;
};
} // namespace remote_control_device