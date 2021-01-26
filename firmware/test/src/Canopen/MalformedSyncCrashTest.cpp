#include "CanopenTestFixture.hpp"

TEST_F(CanopenTest, notCrashing_malformedSYNC)
{
    Canopen co(canIO, log);
    // a pressure sensor had a configuration bug which cased a sync object with
    // length of 8 to be transmitted, crashing the RCD and many other devices
    // this test should ensure that the new RCD shouldn't have a problem with that

    // captured with wireshark while testing with the bike
    Message crashSync = Message_Initializer;
    crashSync.cob_id = 0x80;
    crashSync.rtr = NOT_A_REQUEST;
    crashSync.len = 8;
    crashSync.data[0] = 0x00;
    crashSync.data[1] = 0x81;
    crashSync.data[2] = 0x11;
    crashSync.data[3] = 0x00;
    crashSync.data[4] = 0x00;
    crashSync.data[5] = 0x00;
    crashSync.data[6] = 0x00;
    crashSync.data[7] = 0x00;
    {
        CFLocker locker;
        canDispatch(locker.getOD(), &crashSync);
    }

    // Steering actuator pushed out an EMCY with
    // "Error register: 0x10, Communication error (overrun, error state)"
    // adding this too, maybe this was one failure cause
    Message crashEmcy = Message_Initializer;
    crashEmcy.cob_id = 0xb0;
    crashEmcy.rtr = NOT_A_REQUEST;
    crashEmcy.len = 8;
    crashEmcy.data[0] = 0x40;
    crashEmcy.data[1] = 0x82;
    crashEmcy.data[2] = 0x10;
    crashEmcy.data[3] = 0x1b;
    crashEmcy.data[4] = 0x00;
    crashEmcy.data[5] = 0x00;
    crashEmcy.data[6] = 0x00;
    crashEmcy.data[7] = 0x00;
    {
        CFLocker locker;
        canDispatch(locker.getOD(), &crashEmcy);
    }

    // while executing this test nothing happened and the code evaluating
    // the messages is also written well enough to check against these kind of
    // misformed packets
    // maybe a race condition in the old code caused this to blow up, who knows
}