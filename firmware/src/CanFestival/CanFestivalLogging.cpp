#include "CanFestivalLogging.h"
#include "PeripheralDrivers/TerminalIO.hpp"

using namespace remote_control_device;

#ifndef remote_control_device_TESTING

/*extern "C" void canopen_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    TerminalIO::instance().log().vlog(Logging::Origin::CanFestival, Logging::Level::Info, format,
                                      args);
    va_end(args);
}

extern "C" void canopen_logWarn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    TerminalIO::instance().log().vlog(Logging::Origin::CanFestival, Logging::Level::Warning, format,
                                      args);
    va_end(args);
}

extern "C" void canopen_logErr(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    TerminalIO::instance().log().vlog(Logging::Origin::CanFestival, Logging::Level::Error, format,
                                      args);
    va_end(args);
}*/

#else
#include <cstdio>

extern "C" void canopen_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("%s%s ", Logging::levelToString(Logging::Level::Info),
           Logging::originToString(Logging::Origin::CanFestival));
    vprintf(format, args);
    va_end(args);
}

extern "C" void canopen_logWarn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("%s%s ", Logging::levelToString(Logging::Level::Warning),
           Logging::originToString(Logging::Origin::CanFestival));
    vprintf(format, args);
    va_end(args);
}

extern "C" void canopen_logErr(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("%s%s ", Logging::levelToString(Logging::Level::Error),
           Logging::originToString(Logging::Origin::CanFestival));
    vprintf(format, args);
    va_end(args);
}

#endif