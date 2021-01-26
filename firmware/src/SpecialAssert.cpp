#include "SpecialAssert.hpp"
#include "BuildConfiguration.hpp"


#ifdef BUILDCONFIG_EMBEDDED_BUILD
#include <FreeRTOS.h>
#include <task.h>

const char secret_key[4] __attribute__((section(".fw_crc"))) = {0xDE, 0xAD, 0xBE, 0xEF};

void _specialAssert(bool condition, int line, const char* file)
{
    if (!condition)
    {
        volatile uint64_t myVar = secret_key[1] + 1;
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