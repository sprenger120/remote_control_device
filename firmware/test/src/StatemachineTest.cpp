#include "mock/CanIOMock.hpp"
#include "mock/CanopenMock.hpp"
#include "mock/HALMock.hpp"
#include "mock/HardwareSwitchesMock.hpp"
#include "mock/LEDUpdaterMock.hpp"
#include "mock/LEDsMock.hpp"
#include "mock/RemoteControlMock.hpp"
#include "mock/StateCallbacksMock.hpp"
#include "mock/TerminalIOMock.hpp"
#include "mock/CanFestivalTimersMock.hpp"
#include "gtest/gtest.h"
#include <FreeRTOS.h>
#include <Statemachine/StateSources.hpp>
#include <Statemachine/Statemachine.hpp>
#include <Statemachine/States.hpp>
#include <array>
#include <exception>
#include <iostream>
#include <task.h>

using namespace remote_control_device;
using ::testing::Return;

class StatemachineTest : public ::testing::Test
{
protected:
    StatemachineTest() :
          term(hal),
          log(term, hal),
          canIO(hal, log),
          ledU(canIO),
          cft(hal, log),
            recv(hal, log),
           rc(hal, log, recv),
          co(canIO, log),
          sm(co, rc, hws, ledU, term, hal, iwdg, cft)
    {
    }

    HALMock hal;
    TerminalIOMock term;
    LoggingMock log;
    CanIOMock canIO;

    LEDUpdaterMock ledU;

    CanFestivalTimersMock cft;
    CanopenMock co;
    HardwareSwitchesMock hws;

    ReceiverModuleMock recv;
    RemoteControlMock rc;

    IWDG_HandleTypeDef iwdg;
    Statemachine sm;
};

TEST_F(StatemachineTest, enterStateSetProperties)
{
    auto pSc = new StateCallbacksMock();
    auto & sc = *pSc;

    State states[] = {State(
        /* id */ StateId::Start,
        /* priority */ 255,

        /* brake coupling */ CouplingState::Disengaged,
        /* steering coupling */ CouplingState::Disengaged,

        /* send targets */ false,
        /* steering status */ CanDeviceState::Operational,
        /* brake status */ CanDeviceState::Operational,
        /* drive motor status */ CanDeviceState::Operational,

        /* requires unlock */ false,

     pSc)};

    // dont care about led updater here
    EXPECT_CALL(ledU, update).WillRepeatedly(Return());

    // sm should initialize with NO_STATE
    ASSERT_EQ(sm.getCurrentState(), StateId::NO_STATE);

    /*
     * Test behaviour without any input changes,
     * should change to bootup and stay there
     */
    const int dispatchRepeatCnt = 10;

    EXPECT_CALL(co, update).Times(dispatchRepeatCnt);
    EXPECT_CALL(rc, update).Times(dispatchRepeatCnt);
    EXPECT_CALL(hws, update).Times(dispatchRepeatCnt);

    // check if state switch stuff is executed
    EXPECT_CALL(co, setSelfState(states[0].id)).Times(1);
    EXPECT_CALL(co, setActuatorPDOs(false)).Times(1);
    EXPECT_CALL(co, setDeviceState(Canopen::BusDevices::SteeringActuator, states[0].stateSteering))
        .Times(1);
    EXPECT_CALL(co, setDeviceState(Canopen::BusDevices::BrakeActuator, states[0].stateBrake))
        .Times(1);
    EXPECT_CALL(
        co, setDeviceState(Canopen::BusDevices::DriveMotorController, states[0].stateDriveMotor))
        .Times(1);
    EXPECT_CALL(
        co, setDeviceState(Canopen::BusDevices::BrakePressureSensor, CanDeviceState::Operational))
        .Times(1);
    EXPECT_CALL(co, setCouplingStates(states[0].couplingBrake == CouplingState::Engaged,
                                      states[0].couplingSteering == CouplingState::Engaged))
        .Times(1);

    // check for state functions to be called
    EXPECT_CALL(sc, process).Times(dispatchRepeatCnt);
    EXPECT_CALL(sc, oneTimeSetup).Times(1);
    EXPECT_CALL(sc, checkConditions).Times(dispatchRepeatCnt);

    // Dispatch should setup the system to the stuff defined in states
    for (int i = 0; i < 10; ++i)
    {
        sm._dispatch(std::span(states, 1), states[0]);
        ASSERT_EQ(sm.getCurrentState(), StateId::Start);
    }
}

TEST_F(StatemachineTest, statePriorisationAndUnlock)
{
    const std::array<State, 4> states = {{
          State(
              /* id */ StateId::Idle,
              /* priority */ 0,

              /* brake coupling */ CouplingState::Disengaged,
              /* steering coupling */ CouplingState::Disengaged,

              /* send targets */ false,
              /* steering status */ CanDeviceState::Operational,
              /* brake status */ CanDeviceState::Operational,
              /* drive motor status */ CanDeviceState::Operational,

              /* requires unlock */ false,

               new StateCallbacks(
                   /* process function*/
                   [](StateChaningSources &) -> void {},
                   /* check conditions */
                   [](StateChaningSources &) -> bool { return true; },
                   /* one time setup */
                   [](StateChaningSources &) -> void {}
               )
        ),

        State(
            /* id */ StateId::Start,
            /* priority */ 255,

            /* brake coupling */ CouplingState::Disengaged,
            /* steering coupling */ CouplingState::Disengaged,

            /* send targets */ false,
            /* steering status */ CanDeviceState::Operational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ false,

            new StateCallbacks(
                /* process function*/
                [](StateChaningSources & src) -> void {
                    if (!src.busDevicesState.timeout) {
                        src.sigalStartedUp();
                    }
                },
                /* check conditions */
                [](StateChaningSources & src) -> bool { return !src.isStartedUp(); },
                /* one time setup */
                [](StateChaningSources &) -> void {}
                )
        ),

        State(
            /* id */ StateId::RemoteControl,
            /* priority */ 10,

            /* brake coupling */ CouplingState::Disengaged,
            /* steering coupling */ CouplingState::Disengaged,

            /* send targets */ false,
            /* steering status */ CanDeviceState::Operational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ true,
             new StateCallbacks(
                 /* process function*/
                 [](StateChaningSources &) -> void {},
                 /* check conditions */
                 [](StateChaningSources & src) -> bool { return src.remoteControl.switchRemoteControl; },
                 /* one time setup */
                 [](StateChaningSources &) -> void {}
             )
        ),

        State(
            /* id */ StateId::SoftEmergency,
            /* priority */ 254,

            /* brake coupling */ CouplingState::Disengaged,
            /* steering coupling */ CouplingState::Disengaged,

            /* send targets */ false,
            /* steering status */ CanDeviceState::Operational,
            /* brake status */ CanDeviceState::Operational,
            /* drive motor status */ CanDeviceState::Operational,

            /* requires unlock */ false,

         new StateCallbacks(
             /* process function*/
             [](StateChaningSources &) -> void {},
             /* check conditions */
             [](StateChaningSources & src) -> bool { return src.remoteControl.buttonEmergency; },
             /* one time setup */
             [](StateChaningSources &) -> void {}
            )
        )
    }};
    const auto & idleState = states[0];
    ASSERT_EQ(idleState.priority, States::IDLE_STATE_PRIORITY);
    auto dispatch = [&]() -> void {
        // Shorthand for dispatch call
      sm._dispatch(std::span(states.data(), states.size()), idleState);
    };

    // don't care about setup / setting calls
    EXPECT_CALL(ledU, update).WillRepeatedly(Return());
    EXPECT_CALL(rc, update).WillRepeatedly(Return());
    EXPECT_CALL(co, setSelfState).WillRepeatedly(Return());
    EXPECT_CALL(co, setActuatorPDOs).WillRepeatedly(Return());
    EXPECT_CALL(co, setDeviceState).WillRepeatedly(Return());
    EXPECT_CALL(co, setCouplingStates).WillRepeatedly(Return());

    ASSERT_EQ(sm.getCurrentState(), StateId::NO_STATE);

    EXPECT_CALL(rc, update).WillRepeatedly([](RemoteControlState &state) -> void {
      state.timeout = false;
    });
    EXPECT_CALL(hws, update).WillRepeatedly([](HardwareSwitchesState &state) -> void {
      state.BikeEmergency = false;
    });
    EXPECT_CALL(co, update).WillRepeatedly([](BusDevicesState &state) -> void {
      state.rtdEmergency = false;
      state.timeout = true;
    });

    /* Startup routine */
    // remain in start until bus devices are up
    for (int i=0;i<10;++i)
    {
        dispatch();
        ASSERT_EQ(sm.getCurrentState(), StateId::Start);
    }
    EXPECT_CALL(hws, update).WillRepeatedly(Return());
    EXPECT_CALL(rc, update).WillRepeatedly(Return());
    EXPECT_CALL(co, update).WillRepeatedly([](BusDevicesState &state) -> void {
      state.rtdEmergency = false;
      state.timeout = false;
      state.rtdBootedUp = true;
    });

    // startup now satisfied, dispatch after will switch to idle
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::Start);

    // bootup, remain in idle
    for (int i=0;i<10;++i)
    {
        dispatch();
        ASSERT_EQ(sm.getCurrentState(), StateId::Idle);
    }

    /* Remote Control */
    // not switching until unlock is asserted
    EXPECT_CALL(rc, update).WillOnce([](RemoteControlState &state) -> void {
      state.switchRemoteControl = true;
    });
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::Idle);

    // asserting unlock
    EXPECT_CALL(rc, update).WillOnce([](RemoteControlState &state) -> void {
      state.switchRemoteControl = true;
      state.switchUnlock = true;
    });
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::RemoteControl);

    // stay even when unlock is released
    EXPECT_CALL(rc, update).WillOnce([](RemoteControlState &state) -> void {
      state.switchRemoteControl = true;
      state.switchUnlock = false;
    });
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::RemoteControl);

    // release remote control and return back to idle
    EXPECT_CALL(rc, update).WillOnce([](RemoteControlState &state) -> void {
      state.switchRemoteControl = false;
      state.switchUnlock = false;
    });
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::Idle);

    // switch back to remote for testing overwrite
    EXPECT_CALL(rc, update).WillOnce([](RemoteControlState &state) -> void {
      state.switchRemoteControl = true;
      state.switchUnlock = true;
    });
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::RemoteControl);

    // engaging emergency with everything else still on
    EXPECT_CALL(rc, update).WillOnce([](RemoteControlState &state) -> void {
      state.switchRemoteControl = true;
      state.switchUnlock = true;
      state.buttonEmergency = true;
    });
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::SoftEmergency);

    EXPECT_CALL(rc, update).WillOnce([](RemoteControlState &state) -> void {
      state.switchRemoteControl = false;
      state.switchUnlock = false;
      state.buttonEmergency = false;
    });
    dispatch();
    ASSERT_EQ(sm.getCurrentState(), StateId::Idle);
}