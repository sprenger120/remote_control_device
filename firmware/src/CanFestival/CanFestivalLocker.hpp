#pragma once
#include <FreeRTOS.h>
#include <semphr.h>

extern "C" {
#include <generatedOD/RemoteControlDevice.h>
}

/**
 * @brief Provides an easy to use mutex for canFestival calls.
 *
 */
namespace remote_control_device
{
class CFLocker
{
public:
    CFLocker();
    ~CFLocker();

    /**
     * @brief Retrieves the RCD's object dictionary instance
     * 
     * @return CO_Data* 
     */
    CO_Data* getOD();

    /**
     * @brief Only for testing. Resets the OD to initialisation conditions
     * May break when OD is modified outside of mapped variables 
     * PDO's disable / enable states will be inconsistent, reinitialize them to be safe
     * 
     */
    static void resetOD();
private:
    static SemaphoreHandle_t _mtx;
};
} // namespace remote_control_device