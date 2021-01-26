#include "CanopenTestFixture.hpp"

/**
 * @brief Tests if actuator pdos have been published the correct amount of time and with the
 * Correct content
 *
 * @param msgs
 * @param value
 * @param count
 */
void checkActuatorPDOValues(std::vector<Message> &msgs, float value, int count);


TEST_F(CanopenTest, integrationPDOPublishing)
{
    uint32_t time = 1;

    Message targetMsg = Message_Initializer;
    std::vector<Message> msgs;
    EXPECT_CALL(canIO, canSend).WillRepeatedly([&](Message *m) -> void { msgs.emplace_back(*m); });

    /**
     * check bootup traffic
     */
    EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
    MockRepository mocks;

    // check if RTD_State OD write hack works, check constructor for more info
    mocks.ExpectCallFunc(Canopen::testHook_signalRTDRecovery);
    Canopen co(canIO, log);

    // expected one heartbeat, self state emitted
    expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Bootup, StateId::Start, 1);
    // NOT expecting global reset message
    EXPECT_EQ(extractFrame(msgs, NMT_CobId, targetMsg), 0);
    // two messages, no more as actuator pdos should only be sent when activated
    ASSERT_TRUE(msgs.empty());

    /**
     * check 10 hz heatbeat producing, 10hz self state publishing, no actuator PDOs,
     * self state being changed
     */
    co.setSelfState(StateId::Idle);

    // three producer times should also yield three heartbeats
    for (int i = 0; i <= (Canopen::HeartbeatProducerTime_ms / 10) * 3 + 1; ++i)
    {
        time += 10;
        EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
        cft.dispatch();
    }

    expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Operational, StateId::Idle, 3);
    ASSERT_TRUE(msgs.empty());

    /**
     * check pdo publishing with 50 hz, while nmt, self state still going with 10 hz
     */
    // triggers all pdos again internally, so expected counts down below for 10 seconds of
    // transmission have to be upped by 1
    co.setActuatorPDOs(true);

    // without doing any setX functions, everything should initialized at 0
    for (float value = 0.0f; value < 1.0f;)
    {
        for (int i = 0; i <= (Canopen::HeartbeatProducerTime_ms / 10) * 100; ++i)
        {
            time += 10;
            EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
            cft.dispatch();
        }

        // expected 100 or 101 heartbeats, self states emitted
        expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Operational, StateId::Idle, 100);

        checkActuatorPDOValues(msgs, value,
                               (Canopen::HeartbeatProducerTime_ms * 100) /
                               Canopen::ActuatorPDOEventTime_ms);
        ASSERT_TRUE(msgs.empty());

        value += 0.1f;
        co.setWheelDriveTorque(value);
        co.setBrakeForce(value);
        co.setSteeringAngle(value);
    }

    /**
     * check pdo stop being published after deactivating
     */
    co.setActuatorPDOs(false);

    for (int i = 0; i <= (Canopen::HeartbeatProducerTime_ms / 10) * 100; ++i)
    {
        time += 10;
        EXPECT_CALL(halMock, GetTick).WillRepeatedly(Return(time));
        cft.dispatch();
    }

    // expected 100 or 101 heartbeats, self states emitted
    expectSelfState_Hearbeat(msgs, NMTErrorControl_Heartbeat_Operational, StateId::Idle, 100);
    ASSERT_TRUE(msgs.empty());
}

void checkActuatorPDOValues(std::vector<Message> &msgs, float value, int count)
{
    Message targetMsg = Message_Initializer;

    // brake
    int framesCnt = CanopenTest::extractFrame(msgs, Canopen::TPDO2_BrakeCobId, targetMsg);
    EXPECT_GE(framesCnt, count);
    EXPECT_LE(framesCnt, count + 1);
    EXPECT_EQ(BrakeTargetForce,
              (Canopen::mapValue<float, int32_t>(0.0f, 1.0f, Canopen::BrakeForceRaw_Min,
                                                 Canopen::BrakeForceRaw_Max, value)));
    UNS8 *rawDataBr = reinterpret_cast<UNS8 *>(&BrakeTargetForce);
    CanopenTest::ExpectFrameContent(targetMsg, NOT_A_REQUEST, 2, {rawDataBr[0], rawDataBr[1]});

    // steering
    framesCnt = CanopenTest::extractFrame(msgs, Canopen::TPDO3_SteeringCobId, targetMsg);
    EXPECT_GE(framesCnt, count);
    EXPECT_LE(framesCnt, count + 1);
    EXPECT_EQ(SteeringTargetAngle,
              (Canopen::mapValue<float, int32_t>(
                  -1.0f, 1.0f, Canopen::SteeringAngleRaw_Min, Canopen::SteeringAngleRaw_Max,
                  -value /* inverted value due to pyhsical build */)));
    UNS8 *rawDataSteer = reinterpret_cast<UNS8 *>(&SteeringTargetAngle);
    CanopenTest::ExpectFrameContent(targetMsg, NOT_A_REQUEST, 4,
                       {rawDataSteer[0], rawDataSteer[1], rawDataSteer[2], rawDataSteer[3]});

    // Wheel
    framesCnt = CanopenTest::extractFrame(msgs, Canopen::TPDO4_WheelTorqueCobId, targetMsg);
    EXPECT_GE(framesCnt, count);
    EXPECT_LE(framesCnt, count + 1);
    EXPECT_EQ(WheelTargetTorque,
              (Canopen::mapValue<float, int32_t>(-1.0f, 1.0f, Canopen::WheelDriveTorqueRaw_Min,
                                                 Canopen::WheelDriveTorqueRaw_Max, value)));
    UNS8 *rawDataWheel = reinterpret_cast<UNS8 *>(&WheelTargetTorque);
    CanopenTest::ExpectFrameContent(targetMsg, NOT_A_REQUEST, 2, {rawDataWheel[0], rawDataWheel[1]});

    // All together
    framesCnt = CanopenTest::extractFrame(msgs, Canopen::TPDO5_TargetValues, targetMsg);
    EXPECT_GE(framesCnt, count);
    EXPECT_LE(framesCnt, count + 1);
    CanopenTest::ExpectFrameContent(targetMsg, NOT_A_REQUEST, 8,
                       {rawDataWheel[0], rawDataWheel[1], rawDataBr[0], rawDataBr[1],
                        rawDataSteer[0], rawDataSteer[1], rawDataSteer[2], rawDataSteer[3]});
}