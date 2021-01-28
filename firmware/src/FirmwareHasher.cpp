#include "FirmwareHasher.hpp"
#include "BuildConfiguration.hpp"
#include "SpecialAssert.hpp"
#include "base/hash.hpp"
#include <cmsis_os2.h>

// reserves 8 bits in our special firmwareHash section
// this will be overwritten by the FirmwareHashInserter.py tool
const volatile uint64_t firmwareHash __attribute__((section(".firmwareHash"))) =
    0xDEAD'BEEF'DEAD'BEEF;

// explicit flash address of 'firmwareHash' variable
constexpr uint32_t firmwareHashAddr = 0x800fff8;

namespace remote_control_device
{
uint32_t FirmwareHasher::successfulHashes = {0};

FirmwareHasher::FirmwareHasher()
    : _task(&FirmwareHasher::taskMain, "FWHasher", StackSize, reinterpret_cast<void *>(this),
            osPriority_t::osPriorityLow, wrapper::sync::FirmwareHasher_Ready)
{
    verifyFlash();
}

void FirmwareHasher::verifyFlash()
{
#ifdef BUILDCONFIG_EMBEDDED_BUILD
    const uint64_t hash = bus_node_base::computeFirmwareHash();
    static auto fwHash = reinterpret_cast<uint64_t *>(firmwareHashAddr);

    specialAssert(hash == *fwHash);
    successfulHashes++;
#endif
}

void FirmwareHasher::taskMain(void *context)
{
    auto inst = reinterpret_cast<FirmwareHasher *>(context);
    for (;;)
    {
        inst->verifyFlash();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

} // namespace remote_control_device