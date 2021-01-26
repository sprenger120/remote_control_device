#pragma once
#include <Logging.hpp>
#include "mock/TerminalIOMock.hpp"
#include "mock/HALMock.hpp"

using namespace remote_control_device;

class LoggingMock : public Logging
{
public:
    LoggingMock(TerminalIOMock &terminalIO, HALMock &hal) : Logging(terminalIO, hal)
    {
    }
    /*MOCK_METHOD(void, logDebug, (Origin orig, const char *msg, ...), (override));
    MOCK_METHOD(void, logInfo, (Origin orig, const char *msg, ...), (override));
    MOCK_METHOD(void, logWarning, (Origin orig, const char *msg, ...), (override));
    MOCK_METHOD(void, logError, (Origin orig, const char *msg, ...), (override));
    MOCK_METHOD(void, log, (Origin orig, const char *msg, ...), (override));*/
    MOCK_METHOD(void, vlog, (Origin orig, Level lvl, const char *msg, va_list arg), (override));

    MOCK_METHOD(void, writeRepeatMessageAfterTimeout, (bool), (override));
    MOCK_METHOD(void, writeRepeatMessageAfterTimeout, (), (override));
    MOCK_METHOD(void, disableLogging, (), (override));
};
