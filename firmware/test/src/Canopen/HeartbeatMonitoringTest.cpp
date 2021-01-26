#include "CanopenTestFixture.hpp"

TEST_F(CanopenTest, integrationHeartbeatMonitoring)
{
    uint32_t time = 1;
    Message targetMsg = Message_Initializer;
    BusDevicesState bds;

    std::vector<Message> msgs;
    EXPECT_CALL(canIO, canSend).WillRepeatedly([&](Message *m) -> void { msgs.emplace_back(*m); });
    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
    Canopen co(canIO, log);

    // without any input, all devices assumed timed out / not booted up
    co.update(bds);
    ASSERT_TRUE(bds.timeout);
    ASSERT_FALSE(bds.rtdEmergency);
    ASSERT_FALSE(bds.rtdBootedUp);
    ASSERT_TRUE(co.getRTDTimeout());
    std::cout << "no device booted from start check done\n";

    // test rtd bootup
    {
        CFLocker lock;
        canDispatch(lock.getOD(), &Canopen::RTD_StateBootupMessage);
        co.update(bds);
    }
    ASSERT_FALSE(co.getRTDTimeout());
    ASSERT_FALSE(bds.rtdBootedUp);
    ASSERT_TRUE(bds.timeout);
    std::cout << "rtd bootup check done\n";

    // test rtd ready
    {
        CFLocker lock;
        canDispatch(lock.getOD(), &Canopen::RTD_StateReadyMessage);
        co.update(bds);
    }
    ASSERT_FALSE(co.getRTDTimeout());
    ASSERT_TRUE(bds.rtdBootedUp);
    ASSERT_TRUE(bds.timeout);
    std::cout << "rtd ready check done\n";

    // rtd timing out after 125ms
    for (int i = 0; i <= (Canopen::RTD_RPDOEventTime_ms / 10) * 11; ++i)
    {
        time += 10;
        EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
        cft.dispatch();
    }
    co.update(bds);
    ASSERT_TRUE(co.getRTDTimeout());
    ASSERT_TRUE(bds.rtdBootedUp);
    ASSERT_TRUE(bds.timeout);
    std::cout << "rtd timeout check done\n";

    // signal signal rtd again so it doesnt hinder the timeout flag being reset
    // but use emergency now to check if this flag works
    {
        CFLocker lock;
        canDispatch(lock.getOD(), &Canopen::RTD_StateEmcyMessage);
        co.update(bds);
    }
    ASSERT_FALSE(co.getRTDTimeout());
    ASSERT_TRUE(bds.rtdBootedUp);
    ASSERT_TRUE(bds.rtdEmergency);
    ASSERT_TRUE(bds.timeout);
    std::cout << "rtd emcy check done\n";

    // prepare heartbeat messages
    auto monitoredDevices = co.getMonitoredDevices();
    ASSERT_GT(monitoredDevices.size(), 0);
    Message msgNMTErrCntr = Message_Initializer;
    msgNMTErrCntr.rtr = NOT_A_REQUEST;
    msgNMTErrCntr.len = 1;
    msgNMTErrCntr.data[0] = 1;

    std::vector<Message> heartbeats;
    for (auto dev : monitoredDevices)
    {
        msgNMTErrCntr.cob_id = NMTErrorControl_BaseCobId + static_cast<UNS8>(dev);
        msgNMTErrCntr.data[0] = NMTErrorControl_Heartbeat_Bootup;
        heartbeats.emplace_back(msgNMTErrCntr);
    }

    // bring up all devices one by one
    {
        CFLocker lock;
        for (int i = 0; i < heartbeats.size(); ++i)
        {
            canDispatch(lock.getOD(), &(heartbeats[i]));
            co.update(bds);

            if (i != monitoredDevices.size() - 1)
            {
                ASSERT_TRUE(bds.timeout);
            }
            else
            {
                // all devices now active
                ASSERT_FALSE(bds.timeout);
            }
        }
    }

    // positive test for isDeviceOnline
    for (auto &dev : monitoredDevices)
    {
        ASSERT_TRUE(co.isDeviceOnline(dev));
    }
    std::cout << "bringing up devices check done\n";

    // devices are now operational
    for (Message &msg : heartbeats)
    {
        msg.data[0] = NMTErrorControl_Heartbeat_Operational;
    }

    // simulate devices being online for 1. 10 seconds, 2. dropping one out, 3. coming back
    Message droppedmsg;
    int step = 0;
    while (1)
    {

        {
            CFLocker lock;
            for (int j = 0; j < 100; ++j)
            {
                time += 100;
                EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
                canDispatch(lock.getOD(), &Canopen::RTD_StateEmcyMessage);

                for (int i = 0; i < heartbeats.size(); ++i)
                {
                    canDispatch(lock.getOD(), &(heartbeats[i]));
                }

                cft.dispatch();
                co.update(bds);

                if (step == 0 || step == 2)
                {
                    ASSERT_FALSE(bds.timeout);
                }
                if (step == 1 && j > 5)
                {
                    // timeout after 500ms
                    ASSERT_TRUE(bds.timeout);
                    break;
                }
            }
        }

        if (step == 0)
        {
            droppedmsg = heartbeats[0];
            // drop one out
            heartbeats.erase(heartbeats.begin());
            std::cout << "finished 10 second online test, dropping one out\n";
        }
        if (step == 1)
        {
            // negative test for isDeviceOnline
            ASSERT_FALSE(co.isDeviceOnline(monitoredDevices[0]));

            heartbeats.emplace_back(droppedmsg);
            std::cout << "timeout detected, bringing device online again\n";
        }
        if (step == 2)
        {
            std::cout << "finished 10 second online after readding, all good\n";
            break;
        }
        step++;
    }
    std::cout << "up down up test done\n";
}

TEST_F(CanopenTest, bugRxTPO_Timeout_DeletingTimers)
{
    /**
     * RPDO Timeout didn't clear its internal timer handle upon timeout
     * When timeout was recovered the stack then refreshed the timeout timer
     * Which involves calling DelAlarm and SetAlarm on the internal timer handle.
     * If properly cleared, DelAlarm does nothing. If not it will now delete a random timer
     * that now occupies the spot causing pdos to not fire, heartbeats to not register etc.
     */
    uint32_t time = 1;

    std::vector<Message> msgs;
    EXPECT_CALL(canIO, canSend).WillRepeatedly([&](Message *m) -> void { msgs.emplace_back(*m); });
    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
    Canopen co(canIO, log);

    // Check if there are enough rpdo event timer slots for all rpdos
    ASSERT_GE(Canopen::MaxRPDOEventTimers, static_cast<uint8_t>(Canopen::RPDOIndex::COUNT));

    // After bootup one rpdo timer is assumed to be registered
    {
        CFLocker lock;
        ASSERT_NE(static_cast<TIMER_HANDLE>(lock.getOD()->RxPDO_EventTimers[static_cast<size_t>(
                      Canopen::RPDOIndex::RTD_State)]),
                  TIMER_NONE);
    }

    // Let enough time pass to time out the rpdo
    time += Canopen::RTD_RPDOEventTime_ms + 1;
    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
    cft.dispatch();

    // Now the timer spot should be empty if not the bug will return
    {
        CFLocker lock;
        ASSERT_EQ(static_cast<TIMER_HANDLE>(lock.getOD()->RxPDO_EventTimers[static_cast<size_t>(
            Canopen::RPDOIndex::RTD_State)]),
                  TIMER_NONE);
    }
}