#include "CanopenTestFixture.hpp"

void expectNMT(std::vector<Message> &msgs, Canopen::BusDevices nodeId, uint8_t state)
{
    for (int i = msgs.size() - 1; i >= 0; --i)
    {
        if (msgs[i].cob_id == CanopenTest::NMT_CobId && msgs[i].len == 2 && msgs[i].data[0] == state &&
            msgs[i].data[1] == static_cast<UNS8>(nodeId))
        {
            msgs.erase(msgs.begin() + i);
            return;
        }
    }
    FAIL() << "expectation for NMT not met!";
}

TEST_F(CanopenTest, integrationNodeStateChange)
{
    uint32_t time = 1;
    Message targetMsg = Message_Initializer;

    std::vector<Message> msgs;
    EXPECT_CALL(canIO, canSend).WillRepeatedly([&](Message *m) -> void { msgs.emplace_back(*m); });
    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
    Canopen co(canIO, log);

    // prepare heartbeat messages
    auto stateCntrldDev = co.getStateControlledDevices();
    ASSERT_GT(stateCntrldDev.size(), 0);
    Message msgNMTErrCntr = Message_Initializer;
    msgNMTErrCntr.rtr = NOT_A_REQUEST;
    msgNMTErrCntr.len = 1;
    msgNMTErrCntr.data[0] = 1;

    std::vector<Message> heartbeats;
    for (auto dev : stateCntrldDev)
    {
        msgNMTErrCntr.cob_id = NMTErrorControl_BaseCobId + static_cast<UNS8>(dev);
        msgNMTErrCntr.data[0] = NMTErrorControl_Heartbeat_Bootup;
        heartbeats.emplace_back(msgNMTErrCntr);
    }

    // submit nodes bootup message
    {
        CFLocker locker;
        for (auto e : heartbeats)
        {
            canDispatch(locker.getOD(), &e);
        }
    }

    // default behaviour is to ask all nodes to boot up
    expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Bootup, StateId::Start, 1);
    for (auto dev : stateCntrldDev)
    {
        expectNMT(msgs, dev, NMT_CommandStart);
    }
    ASSERT_TRUE(msgs.empty());
    std::cout << "finished bootup command test\n";

    // nodes are operational now, no further state change request are necessary
    for (Message &msg : heartbeats)
    {
        msg.data[0] = NMTErrorControl_Heartbeat_Operational;
    }
    {
        CFLocker locker;
        for (auto e : heartbeats)
        {
            canDispatch(locker.getOD(), &e);
        }
    }
    ASSERT_TRUE(msgs.empty());
    std::cout << "no further messages after bootup w/o state change request test\n";

    // force a node to preop
    auto selectedDevice = stateCntrldDev[0];
    co.setDeviceState(selectedDevice, CanDeviceState::Preoperational);

    expectNMT(msgs, selectedDevice, NMT_CommandPreoperational);
    ASSERT_TRUE(msgs.empty());

    {
        CFLocker locker;
        heartbeats[0].data[0] = NMTErrorControl_Heartbeat_PreOperational;
        canDispatch(locker.getOD(), &(heartbeats[0]));
    }
    // no further messages should be sent as node switched as expected
    ASSERT_TRUE(msgs.empty());
    std::cout << "successful state change test\n";

    // go to operational again but without the node reacting
    // testing request being sent again
    // assuming device is still preop from test above
    co.setDeviceState(selectedDevice, CanDeviceState::Operational);
    expectNMT(msgs, selectedDevice, NMT_CommandStart);
    ASSERT_TRUE(msgs.empty());

    {
        CFLocker locker;
        canDispatch(locker.getOD(), &(heartbeats[0]));
    }

    expectNMT(msgs, selectedDevice, NMT_CommandStart);
    ASSERT_TRUE(msgs.empty());
    std::cout << "successful state change repeat test\n";
}