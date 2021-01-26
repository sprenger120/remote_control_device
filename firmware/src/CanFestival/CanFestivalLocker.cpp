#include "CanFestivalLocker.hpp"
#include "SpecialAssert.hpp"
#include "BuildConfiguration.hpp"

extern "C" {
    //I really hate do do this but I don't see any other """nice"" way to reset the OD 
    // for testing 
    // the alternative is to use dozends of extern calls that will immediately break
    // when some pdo is changed
    // this at least doesn't break until you add another mapped variable

    // Resetting  will also break if the OD is directly modified 
    // like e.g. modifiying a pdo's inhibit time 
    // Please avoid this for sanity
    #include "generatedOD/RemoteControlDevice.c"
}

namespace remote_control_device
{
SemaphoreHandle_t CFLocker::_mtx = xSemaphoreCreateRecursiveMutex();

CFLocker::CFLocker()
{
    xSemaphoreTakeRecursive(_mtx, portMAX_DELAY);
}

CFLocker::~CFLocker()
{
    xSemaphoreGiveRecursive(_mtx);
}

CO_Data *CFLocker::getOD()
{
    return &RemoteControlDevice_Data;
}

void CFLocker::resetOD()
{
#ifdef BUILDCONFIG_TESTING_BUILD
    WheelTargetTorque = 0x0;   /* Mapped at index 0x2000, subindex 0x00 */
    BrakeTargetForce = 0x0;    /* Mapped at index 0x2001, subindex 0x00 */
    SteeringTargetAngle = 0x0; /* Mapped at index 0x2002, subindex 0x00 */
    SelfState = 0x0;           /* Mapped at index 0x2003, subindex 0x00 */
    RTD_State = 0x0;           /* Mapped at index 0x2004, subindex 0x00 */
    BikeEmergency = 0x0;       /* Mapped at index 0x2005, subindex 0x00 */

    RemoteControlDevice_bDeviceNodeId = 0x00;

    for (TIMER_HANDLE &e : RemoteControlDevice_heartBeatTimers)
    {
        e = TIMER_NONE;
    }

    for (s_PDO_status &e : RemoteControlDevice_PDO_status)
    {
        e = s_PDO_status_Initializer;
    }

    RemoteControlDevice_Data = CANOPEN_NODE_DATA_INITIALIZER(RemoteControlDevice);
#else
    specialAssert(false);
#endif
}
} // namespace remote_control_device