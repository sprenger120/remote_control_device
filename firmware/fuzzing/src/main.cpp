#include <array>
#include <exception>
#include <iostream>

#include <CEncapsulation/HAL.hpp>
#include <CanFestival/CanFestivalLocker.hpp>
#include <CanFestival/CanFestivalTimers.hpp>
#include <FreeRTOS.h>
#include <LEDs.hpp>
#include <PeripheralDrivers/CanIO.hpp>
#include <Statemachine/Canopen.hpp>
#include <Statemachine/HardwareSwitches.hpp>
#include <Statemachine/LEDUpdater.hpp>
#include <Statemachine/RemoteControl.hpp>
#include <Statemachine/StateSources.hpp>
#include <Statemachine/Statemachine.hpp>
#include <fake/Task.hpp>
#include <fstream>
#include <vector>

using namespace remote_control_device;

class RemoteControl_FileControlled : public RemoteControl
{
public:
    RemoteControl_FileControlled(RemoteControlState &target) : target(target)
    {
    }
    virtual ~RemoteControl_FileControlled() = default;
    virtual void update(RemoteControlState &trgt) const final
    {
        trgt = target;
    }

private:
    RemoteControlState &target;
};

class HardwareSwitches_FileControlled : public HardwareSwitches
{
public:
    HardwareSwitches_FileControlled(HardwareSwitchesState &target) : target(target)
    {
    }
    virtual ~HardwareSwitches_FileControlled() = default;
    virtual void update(HardwareSwitchesState &state) const final
    {
        state = target;
    }

private:
    HardwareSwitchesState &target;
};

class MyException : public std::runtime_error
{
public:
    MyException() : std::runtime_error("")
    {
    }
};

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        // printf("The argument supplied is %s\n", argv[1]);
    }
    else if (argc > 2)
    {
        printf("Too many arguments supplied.\n");
        return 0;
    }
    else
    {
        printf("One argument expected.\n");
        return 0;
    }

    // file content sets hardware and rc switches
    // pulled from here
    HardwareSwitchesState hwSwitchesState;
    RemoteControlState rcState;

    // Startup application
    FakeTaskContainer task;

    CanIO canio(task.get(), nullptr);
    LED ledRc(nullptr, 0, nullptr);
    LED ledHw(nullptr, 0, nullptr);
    LEDUpdater ledU(ledHw, ledRc, canio);
    HAL hal;
    CanFestivalTimers cft(hal);
    Canopen co;
    HardwareSwitches_FileControlled hws(hwSwitchesState);
    RemoteControl_FileControlled rc(rcState);
    Statemachine sm(co, rc, hws, ledU);

    // open file, read content
    std::ifstream f(argv[1], std::ifstream::in | std::ifstream::binary);
    size_t filesize = 0;
    if (!f) {
        return -1;
    }

    f.seekg(0, std::ios::end);
    filesize = f.tellg();
    f.seekg(0, std::ios::beg);
    char *data = nullptr;

    if (filesize > 0)
    {
        data = new char[filesize];
        f.read(data, filesize);
    }

    // dispatch once with default values
    sm.dispatch();
    cft.dispatch();

    // parse file and execute what is in it
    auto checkIndex = [&](size_t index) -> void {
        if (index >= filesize)
        {
            throw MyException();
        }
    };
    try
    {
        for (size_t i = 0; i < filesize; ++i)
        {
            //std::cout<<"\t offset: "<<i<<"\n";
            if (data[i] == 0)
            {
                // rc, hardware state change
                checkIndex(++i);
                char state = data[i];
                hwSwitchesState.ManualSwitch = (state & (1 << 0)) > 0;
                hwSwitchesState.BikeEmergency = (state & (1 << 1)) > 0;
                rcState.switchUnlock = (state & (1 << 2)) > 0;
                rcState.switchRemoteControl = (state & (1 << 3)) > 0;
                rcState.switchAutonomous = (state & (1 << 4)) > 0;
                rcState.buttonEmergency = (state & (1 << 5)) > 0;
                rcState.timeout = (state & (1 << 6)) > 0;
                //std::cout<<"rc update\n";
            }
            else if (data[i] > 0)
            {
                // can frame
                Message m = Message_Initializer;

                // length
                checkIndex(++i);
                m.len = data[i];
                if (m.len > 8)
                {
                    continue;
                }

                // rtr
                checkIndex(++i);
                m.rtr = data[i] > 0 ? 1 : 0;

                // canId
                // file is created by hand so everything is big endian
                checkIndex(++i);
                m.cob_id = static_cast<UNS16>(data[i]) << 8;
                checkIndex(++i);
                m.cob_id |= static_cast<UNS16>(data[i]);

                // extract data
                if (m.len > 0)
                {
                    for (int j = m.len - 1; j >= 0; --j)
                    {
                        checkIndex(++i);
                        m.data[j] = data[i];
                    }
                }
                CFLocker locker;
                canDispatch(locker.getOD(), &m);
                //std::cout<<"can update\n";
            } else {
                //std::cout<<"unknown \n";
                return -1;
            }

            // calls HardwareSwitches_FileControlled.update (+rc_filecontrolled.update)
            // which will grab our modified states
            sm.dispatch();
            cft.dispatch();
        }
    }
    catch (const MyException &myEx)
    {
        // no worries, expected exception
        // fuzzer does all kinds of crazy things with the file so anything goes
    }

    if (data)
    {
        delete[] data;
    }
    //std::cout<<"all done :)\n";
    return 0;
}