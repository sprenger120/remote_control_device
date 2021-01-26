#pragma once
#include <stdbool.h> // NOLINT  

/**
 * @brief Asserts the given condition, aborts programm if false
 * When compiled for embedded, disables interrupts and enters endless loop 
 * locking up the scheduler until the watchdog resets the device. 
 * For testing does nothing.
 * 
 * @param condition 
 */

#ifdef __cplusplus
extern "C" {
#endif

void _specialAssert(bool condition, int line, const char* file);

#ifdef __cplusplus
}
#endif

#define specialAssert(cond) _specialAssert(cond, __LINE__, __FILE__) //NOLINT 



