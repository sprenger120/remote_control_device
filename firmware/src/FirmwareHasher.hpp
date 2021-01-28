#pragma once

#include "Wrapper/Task.hpp"

namespace remote_control_device
{
class FirmwareHasher
{
public:
    static constexpr uint32_t StackSize = 75;
    FirmwareHasher();

    void verifyFlash();

    static uint32_t successfulHashes;
private:
    wrapper::Task _task;
    
    static void taskMain(void *context);
};
} // namespace remote_control_device