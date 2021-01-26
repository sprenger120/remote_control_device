#include <FreeRTOS.h>
#include <task.h>

namespace std
{
void __throw_bad_function_call()
{
}

void __throw_out_of_range_fmt(char const *, ...)
{
}

void terminate()
{
    taskDISABLE_INTERRUPTS();
#ifdef DEBUG
    __asm("bkpt");
#endif
    for (;;)
    {
    }
}
} // namespace std