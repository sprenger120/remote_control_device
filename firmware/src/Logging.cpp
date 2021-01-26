#include "Logging.hpp"
#include "Application.hpp"
#include "PeripheralDrivers/TerminalIO.hpp"
#include "SpecialAssert.hpp"
#include "Wrapper/HAL.hpp"
#include <cstdio>
#include <cstring>
#include <stm32f3xx_hal.h>

namespace remote_control_device
{

Logging::Logging(TerminalIO &terminalIO, wrapper::HAL &hal) : _terminalIO(terminalIO), _hal(hal)
{
    _mtx = xSemaphoreCreateRecursiveMutex();
    _lastLogTimestamp = hal.GetTick();
}

Logging::~Logging()
{
    vSemaphoreDelete(_mtx);
}

void Logging::logDebug(Logging::Origin orig, const char *msg, ...)
{
    va_list args; // NOLINT
    va_start(args, msg);
    vlog(orig, Logging::Level::Debug, msg, args);
    va_end(args);
}

void Logging::logInfo(Logging::Origin orig, const char *msg, ...)
{
    va_list args; // NOLINT
    va_start(args, msg);
    vlog(orig, Logging::Level::Info, msg, args);
    va_end(args);
}

void Logging::logWarning(Logging::Origin orig, const char *msg, ...)
{
    va_list args; // NOLINT
    va_start(args, msg);
    vlog(orig, Logging::Level::Warning, msg, args);
    va_end(args);
}

void Logging::logError(Logging::Origin orig, const char *msg, ...)
{
    va_list args; // NOLINT
    va_start(args, msg);
    vlog(orig, Logging::Level::Error, msg, args);
    va_end(args);
}

void Logging::log(Origin orig, Level lvl, const char *format, ...)
{
    va_list args; // NOLINT
    va_start(args, format);
    vlog(orig, lvl, format, args);
    va_end(args);
}

void Logging::vlog(Origin orig, Level lvl, const char *format, va_list args)
{
    if (_disabled)
    {
        return;
    }

    if (xSemaphoreTakeRecursive(_mtx, MAX_WAITTIME) == pdFAIL)
    {
        return;
    }

    // format current message
    const int sizeMsgStart{
        snprintf(_printBuffer, BUFFER_SIZE, "%s%s ", levelToString(lvl), originToString(orig))};
    if (sizeMsgStart < 0)
    {
        _terminalIO.write(FORMING_ERROR_MSG);
        return;
    }

    const int sizeMsgBody{
        vsnprintf(_printBuffer + sizeMsgStart, BUFFER_SIZE - sizeMsgStart, format, args)};
    if (sizeMsgBody < 0)
    {
        _terminalIO.write(FORMING_ERROR_MSG);
        return;
    }

    if (static_cast<size_t>(sizeMsgStart + sizeMsgBody + 3) > BUFFER_SIZE)
    {
        _terminalIO.write("Message too large!\r\n\0");
        return;
    }
    else
    {
        strncpy(_printBuffer + sizeMsgStart + sizeMsgBody, "\r\n\0", 3);
    }

    // check if duplicate
    if (strncmp(_printBuffer, _lastLogText, BUFFER_SIZE) == 0)
    {
        _lastLogRepeats++;
    }
    else
    {
        // store this message as template for comparison, also save it
        // from being overwritten by writeRepeatMessageAfterTimeout's buffer usage
        strncpy(_lastLogText, _printBuffer, BUFFER_SIZE);
        writeRepeatMessageAfterTimeout(true);
        _lastLogRepeats = 0;
        // if buffer is chosen so small that not even the message too large message fits
        // no \0 is added by strncpy,  printf and derivates on the other hand will add \0
        _lastLogText[BUFFER_SIZE - 1] = '\0';

        const size_t len{std::min(strlen(_lastLogText), BUFFER_SIZE)};
        _terminalIO.write(std::span(_lastLogText, len));

        _lastLogTimestamp = _hal.GetTick();
    }

    xSemaphoreGiveRecursive(_mtx);
}

void Logging::writeRepeatMessageAfterTimeout()
{
    writeRepeatMessageAfterTimeout(false);
}

void Logging::writeRepeatMessageAfterTimeout(bool force)
{
    if (_lastLogRepeats == 0 || _disabled)
    {
        return;
    }
    static constexpr uint32_t MS_TO_SEC = 1000;

    if (force || (_hal.GetTick() - _lastLogTimestamp >= REPEAT_MSG_TIMEOUT_SEC * MS_TO_SEC))
    {
        if (xSemaphoreTakeRecursive(_mtx, MAX_WAITTIME) == pdFAIL)
        {
            return;
        }

        const int size{snprintf(_printBuffer, BUFFER_SIZE,
                                "... was repeated %d more times the last %d second(s)\r\n",
                                _lastLogRepeats, REPEAT_MSG_TIMEOUT_SEC)};
        if (size > 0)
        {
            _terminalIO.write(std::span(_printBuffer, size));
        }
        else
        {
            _terminalIO.write(FORMING_ERROR_MSG);
        }

        _lastLogTimestamp = _hal.GetTick();
        _lastLogRepeats = 0;
        xSemaphoreGiveRecursive(_mtx);
    }
}

const char *Logging::levelToString(Level lvl)
{
    switch (lvl)
    {
        case Level::Debug:
            return "[DEBUG]";
        case Level::Info:
            return "[INFO]";
        case Level::Warning:
            return "[WARNING]";
        case Level::Error:
            return "[ERROR]";
        default:
            return "[UNKNOWN]";
    }
}

const char *Logging::originToString(Origin orig)
{
    switch (orig)
    {
        case Origin::CanIO:
            return "[CanIO]";
        case Origin::CFTimers:
            return "[CF Timers]";
        case Origin::Led:
            return "[LED]";
        case Origin::RadioControl:
            return "[Radio control]";
        case Origin::StateMachine:
            return "[State Machine]";
        case Origin::BusDevices:
            return "[Bus Devices]";
        case Origin::CMD:
            return "[Commands]";
        case Origin::CanFestival:
            return "[CanFestival]";
        case Origin::General:
            return "[General]";
        default:
            return "[UNKNOWN]";
    }
}
} // namespace remote_control_device