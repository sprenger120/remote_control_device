#include "SpecialAssert.hpp"
#include "BuildConfiguration.hpp"

#ifdef BUILDCONFIG_EMBEDDED_BUILD
#include <FreeRTOS.h>
#include <task.h>
void _specialAssert(bool condition, int line, const char* file)
{
    if (!condition)
    {
        taskDISABLE_INTERRUPTS();
#ifdef DEBUG
        __asm("bkpt");
#endif
        for (;;)
        {
        }
    }
}
#else
#include <stdexcept>
#include <sstream>
void _specialAssert(bool condition, int line, const char* file)
{
    if (!condition)
    {
        std::stringstream ss;
        ss<<"Assertion Failed "<<file<<":"<<line;
        throw std::runtime_error(ss.str());
    }
}
#endif