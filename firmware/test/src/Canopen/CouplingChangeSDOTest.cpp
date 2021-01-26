#include "CouplingChangeSDOTest.hpp"

TEST_F(CanopenTest, integrationCouplingChange)
{
    uint32_t time = 1;
    Message targetMsg = Message_Initializer;

    std::vector<Message> msgs;
    EXPECT_CALL(canIO, canSend).WillRepeatedly([&](Message *m) -> void { msgs.emplace_back(*m); });
    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
    Canopen co(canIO, log);

    expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Bootup, StateId::Start, 1);

    // test repeated sucessful SDOs
    bool brakeState = true;
    bool steeringState = true;
    Message sdoOK_Brake = constructSDOResponse(Canopen::BrakeActuatorCoupling_IndexOD,
                                               Canopen::BrakeActuatorCoupling_SubIndexOD,
                                               Canopen::BusDevices::BrakeActuator);

    Message sdoOK_Steering = constructSDOResponse(Canopen::SteeringActuatorCoupling_IndexOD,
                                                  Canopen::SteerinActuatorCoupling_SubIndexOD,
                                                  Canopen::BusDevices::SteeringActuator);

    ASSERT_EQ(Canopen::HeartbeatProducerTime_ms, Canopen::SelfState_TPDOEventTime_ms);
    ASSERT_GT(SDO_TIMEOUT_MS, Canopen::HeartbeatProducerTime_ms * 2);

    for (int i = 0; i < 100; ++i)
    {
        co.setCouplingStates(brakeState, steeringState);
        assertSDORequest(msgs, brakeState, steeringState);

        // some delay that is lower than timeout
        for (int j = 0; j < (Canopen::HeartbeatProducerTime_ms / 10) * 2; ++j)
        {
            time += 10;
            EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
            cft.dispatch();
        }
        expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Operational, StateId::Start, -1);

        {
            CFLocker lock;
            canDispatch(lock.getOD(), &sdoOK_Steering);
            canDispatch(lock.getOD(), &sdoOK_Brake);
        }

        brakeState = !brakeState;
        steeringState = !steeringState;
    }

    std::cout << "finished SDOs without timeout test\n";

    // test state change mid transmission
    co.setCouplingStates(brakeState, steeringState);
    assertSDORequest(msgs, brakeState, steeringState);

    int state = 0;
    while (state < 2)
    {
        // first iteration watches for transfer being started after its through because state
        // changed second one watches for no new transfer because of no state change

        // so some heartbeats are emitted for expectSelfState_Hearbeat
        ASSERT_LT(Canopen::SelfState_TPDOEventTime_ms, SDO_TIMEOUT_MS / 2);

        // some delay that is lower than sdo timeout
        for (int j = 0; j < (SDO_TIMEOUT_MS / 10) / 2; ++j)
        {
            time += 10;
            EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
            cft.dispatch();
            co.setCouplingStates(!brakeState, !steeringState);
        }
        expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Operational, StateId::Start, -1);

        // there should be no new requests while these two are running
        ASSERT_TRUE(msgs.empty());

        // even when there are more lines available
        ASSERT_GT(SDO_MAX_SIMULTANEOUS_TRANSFERS, 2);

        // confirm transfer
        {
            CFLocker lock;
            canDispatch(lock.getOD(), &sdoOK_Steering);
            canDispatch(lock.getOD(), &sdoOK_Brake);
        }

        if (state == 0)
        {
            assertSDORequest(msgs, !brakeState, !steeringState);
        }
        if (state == 1)
        {
            ASSERT_TRUE(msgs.empty());
        }
        std::cout << "finished iteration:" << state << "\n";
        state++;
    }

    // test sdo timeout and retransmission after
    co.setCouplingStates(brakeState, steeringState);
    assertSDORequest(msgs, brakeState, steeringState);
    for (int i = 0; i < 100; ++i)
    {
        // some delay that is lower than timeout
        for (int j = 0; j < (SDO_TIMEOUT_MS / 10); ++j)
        {
            time += 10;
            EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
            cft.dispatch();
        }
        expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Operational, StateId::Start, -1);

        assertSDOAbortTransfer(msgs);
        assertSDORequest(msgs, brakeState, steeringState);
        ASSERT_TRUE(msgs.empty());
    }

    std::cout << "finished SDOs timeout test\n";
}



void assertSDORequest(std::vector<Message> &msgs, bool brakeCouplingTarget,
                      bool steeringCouplingTarget)
{
    ASSERT_EQ(msgs.size(), 2);
    bool brakeCouplingParsed = false;
    bool steeringCouplingParsed = false;
    SDORequestFlagByte fb;
    for (int i = 0; i < 2; ++i)
    {
        // analyze status flag
        fb.raw = msgs[i].data[0];

        ASSERT_EQ(fb.flags.clientCommandSpecifier, INITIATE_DOWNLOAD_REQUEST);
        // data fits into request -> expedited transfer, size indication is required
        ASSERT_EQ(fb.flags.transferType, 1);
        ASSERT_EQ(fb.flags.sizeIndicator, 1);
        // writing 32 bit value, 4 bytes allowed in exp. transfer - 4 bytes used = 0
        ASSERT_EQ(fb.flags.nonDataBytesCount, 0);
        ASSERT_EQ(fb.flags.reservedAlwaysZero, 0);

        uint16_t odIndex =
            static_cast<uint16_t>(msgs[i].data[1]) | (static_cast<uint16_t>(msgs[i].data[2]) << 8);
        uint16_t odSubIndex = msgs[i].data[3];
        uint32_t data = static_cast<uint16_t>(msgs[i].data[4]) |
                        (static_cast<uint16_t>(msgs[i].data[5]) << 8) |
                        (static_cast<uint16_t>(msgs[i].data[6]) << 16) |
                        (static_cast<uint16_t>(msgs[i].data[7]) << 24);

        if (msgs[i].cob_id ==
            CanopenTest::SDO_Request_BaseCobId + static_cast<uint16_t>(Canopen::BusDevices::BrakeActuator))
        {
            ASSERT_EQ(odIndex, Canopen::BrakeActuatorCoupling_IndexOD);
            ASSERT_EQ(odSubIndex, Canopen::BrakeActuatorCoupling_SubIndexOD);
            if (brakeCouplingTarget)
            {
                ASSERT_EQ(data, Canopen::BrakeActuatorCoupling_EngagedValue);
            }
            else
            {
                ASSERT_EQ(data, Canopen::BrakeActuatorCoupling_DisEngagedValue);
            }
            brakeCouplingParsed = true;
        }
        else if (msgs[i].cob_id ==  CanopenTest::SDO_Request_BaseCobId +
                                   static_cast<uint16_t>(Canopen::BusDevices::SteeringActuator))
        {
            ASSERT_EQ(odIndex, Canopen::SteeringActuatorCoupling_IndexOD);
            ASSERT_EQ(odSubIndex, Canopen::SteerinActuatorCoupling_SubIndexOD);
            if (steeringCouplingTarget)
            {
                ASSERT_EQ(data, Canopen::SteerinActuatorCoupling_EngagedValue);
            }
            else
            {
                ASSERT_EQ(data, Canopen::SteerinActuatorCoupling_DisEngagedValue);
            }

            steeringCouplingParsed = true;
        }
        else
        {
            FAIL() << "Unknown SDO request";
        }
    }
    ASSERT_TRUE(steeringCouplingParsed);
    ASSERT_TRUE(brakeCouplingParsed);
    msgs.clear();
}


Message constructSDOResponse(uint16_t index, uint8_t subIndex, Canopen::BusDevices dev)
{
    SDOResponseFlagByte fl;
    fl.flags.reservedAlwaysZero = 0;
    fl.flags.commandSpecifier = INITIATE_DOWNLOAD_RESPONSE;

    Message m;
    m.cob_id =  CanopenTest::SDO_Response_BaseCobId + static_cast<uint8_t>(dev);
    m.rtr = NOT_A_REQUEST;
    m.len = 8;
    m.data[0] = fl.raw;
    m.data[1] = static_cast<uint8_t>(index & 0xFF);
    m.data[2] = static_cast<uint8_t>((index >> 8));
    m.data[3] = subIndex;
    m.data[4] = 0;
    m.data[5] = 0;
    m.data[6] = 0;
    m.data[7] = 0;
    return m;
}

void assertSDOAbortTransfer(std::vector<Message> &msgs)
{
    ASSERT_GE(msgs.size(), 2);
    bool brakeCouplingParsed = false;
    bool steeringCouplingParsed = false;

    SDOResponseFlagByte fb;

    for (int i = msgs.size(); i >= 0; --i)
    {
        // analyze status flag
        fb.raw = msgs[i].data[0];
        if (fb.flags.commandSpecifier != ABORT_TRANSFER_REQUEST)
        {
            continue;
        }
        ASSERT_EQ(fb.flags.reservedAlwaysZero, 0);

        uint16_t odIndex =
            static_cast<uint16_t>(msgs[i].data[1]) | (static_cast<uint16_t>(msgs[i].data[2]) << 8);
        uint16_t odSubIndex = msgs[i].data[3];
        uint32_t data = static_cast<uint16_t>(msgs[i].data[4]) |
                        (static_cast<uint16_t>(msgs[i].data[5]) << 8) |
                        (static_cast<uint16_t>(msgs[i].data[6]) << 16) |
                        (static_cast<uint16_t>(msgs[i].data[7]) << 24);

        ASSERT_EQ(data, SDOABT_TIMED_OUT);

        if (msgs[i].cob_id ==
            CanopenTest::SDO_Request_BaseCobId + static_cast<uint16_t>(Canopen::BusDevices::BrakeActuator))
        {
            if (brakeCouplingParsed)
            {
                continue;
            }
            ASSERT_EQ(odIndex, Canopen::BrakeActuatorCoupling_IndexOD);
            ASSERT_EQ(odSubIndex, Canopen::BrakeActuatorCoupling_SubIndexOD);
            brakeCouplingParsed = true;
        }
        else if (msgs[i].cob_id ==  CanopenTest::SDO_Request_BaseCobId +
                                   static_cast<uint16_t>(Canopen::BusDevices::SteeringActuator))
        {
            if (steeringCouplingParsed)
            {
                continue;
            }
            ASSERT_EQ(odIndex, Canopen::SteeringActuatorCoupling_IndexOD);
            ASSERT_EQ(odSubIndex, Canopen::SteerinActuatorCoupling_SubIndexOD);
            steeringCouplingParsed = true;
        }
        else
        {
            FAIL() << "Unknown SDO request";
        }
        msgs.erase(msgs.begin() + i);
    }
    ASSERT_TRUE(steeringCouplingParsed);
    ASSERT_TRUE(brakeCouplingParsed);
}


