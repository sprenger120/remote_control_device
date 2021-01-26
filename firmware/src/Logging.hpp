#pragma once
#include <FreeRTOS.h>
#include <array>
#include <cstdarg>
#include <semphr.h>

namespace wrapper
{
class HAL;
}

namespace remote_control_device
{
class TerminalIO;

class Logging
{
public:
    /**
     * @brief Construct a new Logging object
     *
     * @param terminalIO instance of terminalIO to write to
     * @param hal instalce
     */
    Logging(TerminalIO &terminalIO, wrapper::HAL &hal);
    virtual ~Logging();

    Logging(const Logging &) = delete;
    Logging(Logging &&) = delete;
    Logging &operator=(const Logging &) = delete;
    Logging &operator=(Logging &&) = delete;

    enum class Level : uint8_t
    {
        Debug,
        Info,
        Warning,
        Error
    };

    enum class Origin : uint8_t
    {
        CanIO,
        CFTimers,
        Led,
        RadioControl,
        StateMachine,
        BusDevices,
        CMD,
        CanFestival,
        General
    };

    /**
     * @brief Writes a log message to be sent via TerminalIO
     * Has light flood protection against same messages. Do not call from ISR
     *
     * @param orig Module log originates from
     * @param lvl Importance
     * @param msg printf style message
     * @param ...  parameters to fill into message
     */
    virtual void logDebug(Origin orig, const char *msg, ...);
    virtual void logInfo(Origin orig, const char *msg, ...);
    virtual void logWarning(Origin orig, const char *msg, ...);
    virtual void logError(Origin orig, const char *msg, ...);
    virtual void log(Origin orig, Level lvl, const char *msg, ...);
    virtual void vlog(Origin orig, Level lvl, const char *msg, va_list arg);

    /**
     * @brief Call periodically to send the "Repeated n times..." message
     * Message is only printed every REPEAT_MSG_TIMEOUT time or
     * or new, different log is submitted
     *
     * @param force true bypasses timeout check
     */
    virtual void writeRepeatMessageAfterTimeout();
    virtual void writeRepeatMessageAfterTimeout(bool force);

    /**
     * @brief Makes log, vlog, writeRepeatMessageAfterTimeout do nothing
     *
     * @return true
     * @return false
     */
    virtual void disableLogging()
    {
        _disabled = true;
    }

    /**
     * @brief Max allowed size per log
     *
     */
    static constexpr size_t BUFFER_SIZE = 100;

    /**
     * @brief Max waittime for log mutex
     *
     */
    static constexpr TickType_t MAX_WAITTIME = pdMS_TO_TICKS(10);

    /**
     * @brief See writeRepeatMessageAfterTimeout description
     *
     */
    static constexpr uint8_t REPEAT_MSG_TIMEOUT_SEC = 1; // seconds

    static const char *originToString(Origin orig);
    static const char *levelToString(Level lvl);

    static constexpr const char *FORMING_ERROR_MSG = "Forming message failed\r\n";

private:
    char _printBuffer[BUFFER_SIZE] = {0}; // NOLINT
    SemaphoreHandle_t _mtx;
    TerminalIO &_terminalIO;
    wrapper::HAL &_hal;
    bool _disabled{false};

    char _lastLogText[BUFFER_SIZE] = {0}; // NOLINT
    uint16_t _lastLogRepeats = 0;
    uint32_t _lastLogTimestamp;
};
} // namespace remote_control_device