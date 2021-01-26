#pragma once
#include "gmock/gmock.h"
#include <Statemachine/StateSources.hpp>
#include <Statemachine/Canopen.hpp>
#include "mock/CanIOMock.hpp"
#include "mock/LoggingMock.hpp"

using namespace remote_control_device;

class CanopenMock : public Canopen
{
public:
    CanopenMock(CanIO &canio, LoggingMock &log) : Canopen(canio, log, true){};

    MOCK_METHOD(void, setTPDO, (const Canopen::TPDOIndex, bool), (override));
    MOCK_METHOD(void, setActuatorPDOs, (bool), (override));
    MOCK_METHOD(void, setDeviceState, (const Canopen::BusDevices, CanDeviceState),
                (override));
    MOCK_METHOD(void, setSelfState, (const StateId), (override));
    MOCK_METHOD(void, setCouplingStates, (bool, bool), (override));
    MOCK_METHOD(void, update, (BusDevicesState &), (override));
    MOCK_METHOD(void, setWheelDriveTorque, (float), (override));
    MOCK_METHOD(void, setBrakeForce, (float), (override));
    MOCK_METHOD(void, setSteeringAngle, (float), (override));
    MOCK_METHOD(void, signalRTDTimeout, (), (override));
    MOCK_METHOD(void, signalRTDRecovery, (), (override));
    MOCK_METHOD(bool, getRTDTimeout, (), (override));
    MOCK_METHOD(bool, isDeviceOnline, (const Canopen::BusDevices), (override, const));

    /*MOCK_METHOD(std::array<Canopen::BusDevices, Canopen::MonitoredDeviceCount>,
    getMonitoredDevices, (bool), (override, const)); MOCK_METHOD(std::array<Canopen::BusDevices,
    Canopen::StateControlledDeviceCount>, getStateControlledDevices, (bool), (override, const));*/
};