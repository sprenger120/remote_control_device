#pragma once
#include "SpecialAssert.hpp"
#include "State.hpp"
#include "StateSources.hpp"
#include "Statemachine.hpp"
#include <FreeRTOS.h>
#include <algorithm>
#include <array>
#include <memory>
#include <semphr.h>

extern "C"
{
#include <canfestival/data.h>
#include <canfestival/def.h>
}

/**
 * @brief Abstracts canfestival / canopen processes for usage in state machine
 *
 */

namespace remote_control_device
{
class CanIO;
class Logging;
class TerminalIO;

class Canopen
{
public:
    /**
     * @brief Construct a new Canopen object
     *
     * @param bypass true to not initialize canfestival, internals structures
     */
    explicit Canopen(CanIO &canio, Logging &log, bool bypass = false);
    virtual ~Canopen();

    Canopen(const Canopen &) = delete;
    Canopen(Canopen &&) = delete;
    Canopen &operator=(const Canopen &) = delete;
    Canopen &operator=(Canopen &&) = delete;

    // references to RCD's OD content
    // this definitely is data duplication but
    // its way easier to just write the values a second time
    // and have them static than to retrieve them in the constructor
    // and having to do a bunch of work in the intergration test support functions
    static constexpr uint16_t RTD_State_ODIndex = 0x2004;
    static constexpr uint8_t RTD_State_ODSubIndex = 0x0;

    static constexpr uint16_t TPDO1_BaseCobId = 0x180; // node id is added later
    static constexpr uint16_t TPDO2_BrakeCobId = 0x220;
    static constexpr uint16_t TPDO3_SteeringCobId = 0x230;
    static constexpr uint16_t TPDO4_WheelTorqueCobId = 0x210;
    static constexpr uint16_t TPDO5_TargetValues = 0x200;
    static constexpr uint16_t RPDO1_RTD_State = 0x182;

    static constexpr uint16_t HeartbeatConsumerTimeout_ms = 500;
    static constexpr uint16_t HeartbeatProducerTime_ms = 100;
    static constexpr uint16_t ActuatorPDOEventTime_ms = 20;
    static constexpr uint16_t RTD_RPDOEventTime_ms = 125;
    static constexpr uint16_t SelfState_TPDOEventTime_ms = 100;

    static constexpr uint8_t MaxRPDOEventTimers = 2;

    /**
     * @brief TPDO indexes corresponding to configuration in object dictionary
     *
     */
    enum class TPDOIndex : uint8_t
    {
        // specification counts from 1,  canfestival from 0
        SelfState = 0,
        BrakeForce = 1,
        SteeringAngle = 2,
        MotorTorque = 3,
        TargetValues = 4,
        COUNT
    };

    enum class RPDOIndex : uint8_t
    {
        // specification counts from 1,  canfestival from 0
        RTD_State = 0,
        COUNT
    };

    /**
     * @brief Blocking. Enables / disables a specific pdo
     * Enabling causes the pdos to be sent once regardless of timeout
     *
     * @param index TPDOIndex entry
     * @param enable true for enabling, false otherwise
     */
    virtual void setTPDO(const TPDOIndex index, bool enable);

    /**
     * @brief Blocking. Enables / disables all actuator pdos
     *
     * @param enable true for enabling, false otherwise
     */
    virtual void setActuatorPDOs(bool enable);

    /**
     * @brief List of all available bus devices. Not all devices listed here are monitored / state
     * controlled
     *
     */
    enum class BusDevices : uint8_t
    {
        RemoteControlDevice = 0x01,
        RealTimeDevice = 0x02,
        DriveMotorController = 0x10,
        WheelSpeedSensor = 0x11,
        BrakeActuator = 0x20,
        BrakePressureSensor = 0x21,
        SteeringActuator = 0x30,
        SteeringAngleSensor = 0x31,
        NO_DEVICE = 0xff
    };

    /**
     * @brief Internal representation of a (timeout) monitored device
     *
     */
    struct MonitoredDevice
    {
        const BusDevices device;
        bool disconnected = true;
        explicit MonitoredDevice(const BusDevices dev) : device(dev)
        {
        }
    };

    /**
     * @brief Checks whether a bus device is online
     * Will return false when device is not a monitored device
     *
     * @param device
     */
    virtual bool isDeviceOnline(const BusDevices device) const;

    /**
     * @brief Internal representaion of a state controlled device
     *
     */
    struct StateControlledDevice
    {
        const BusDevices device;
        CanDeviceState targetState;
        CanDeviceState currentState;

        StateControlledDevice(const BusDevices dev, CanDeviceState target)
            : device(dev), targetState(target), currentState(CanDeviceState::Unknown)
        {
        }
    };

    /**
     * @brief Blocking. Changes a state controlled device's state. Only works with devices
     * in StateControlledDevice list. Will automatically change state even if request is still
     * running.
     *
     * @param device BusDevices
     * @param state CanDeviceState
     */
    virtual void setDeviceState(const BusDevices device, CanDeviceState state);

    /**
     * @brief Blocking. Sets the 'SelfState' PDO value
     *
     */
    virtual void setSelfState(const StateId);

    /**
     * @brief Internal representation of a coupling.
     * Assumes that a coupling can be controlled by sending a SDO request with
     * given parameters.
     *
     */
    struct Coupling
    {
        BusDevices device;
        uint16_t odIndex;
        uint8_t odSubIndex;
        bool targetState = false;
        bool stateForThisRequest = false;
        uint32_t engagedValue;
        uint32_t disengagedValue;

        Coupling(BusDevices device, uint16_t odIndex, uint8_t odSubIndex, uint32_t engagedValue,
                 uint32_t disengagedValue);
    };
    static constexpr uint16_t BrakeActuatorCoupling_IndexOD = 0x60FE;
    static constexpr uint8_t BrakeActuatorCoupling_SubIndexOD = 0x1;
    static constexpr uint32_t BrakeActuatorCoupling_EngagedValue = 0x00010000;
    static constexpr uint32_t BrakeActuatorCoupling_DisEngagedValue = 0x0;

    static constexpr uint16_t SteeringActuatorCoupling_IndexOD = 0x60FE;
    static constexpr uint8_t SteerinActuatorCoupling_SubIndexOD = 0x1;
    static constexpr uint32_t SteerinActuatorCoupling_EngagedValue = 0x00010000;
    static constexpr uint32_t SteerinActuatorCoupling_DisEngagedValue = 0x0;

    /**
     * @brief Blocking. Sets the state of both couplings.
     *
     * @param brake
     * @param steering
     */
    virtual void setCouplingStates(bool brake, bool steering);

    /**
     * @brief RTD_State RPDO values relevant to this device
     *
     */
    static constexpr uint8_t RTD_State_Bootup = 0;
    static constexpr uint8_t RTD_State_Ready = 1;
    static constexpr uint8_t RTD_State_Emergency = 6;

    /**
     * @brief Blocking. Updates state machine's information about bus devices
     *
     */
    virtual void update(BusDevicesState &);

    /**
     * @brief Draws information about monitored devices and state controlled devices to terminal
     *
     * @param term terminalIO instance
     */
    virtual void drawUIDevicesPart(TerminalIO &term);

    /**
     * @brief Quick values used in emergency states
     *
     */
    static constexpr float NoWheelDriveTorque = 0;
    static constexpr float MaxBrakePressure = 1;

    /**
     * @brief Blocking. Sets drive controllers torque.
     * Input is clipped to available range
     *
     * @param v -1 to 1 Negative values implicate backwards driving
     */
    virtual void setWheelDriveTorque(float v);

    /**
     * @brief Used in converting internal value range to raw values sent over the bus
     *
     */
    static constexpr INTEGER16 WheelDriveTorqueRaw_Min = -2200;
    static constexpr INTEGER16 WheelDriveTorqueRaw_Max = 2200;

    /**
     * @brief Blocking. Sets the brake force.
     * Input is clipped to available range
     *
     * @param v 0 to 1 Zero being no braking
     */
    virtual void setBrakeForce(float v);

    /**
     * @brief Used in converting internal value range to raw values sent over the bus
     *
     */
    static constexpr INTEGER16 BrakeForceRaw_Min = 5000;
    static constexpr INTEGER16 BrakeForceRaw_Max = 14000;

    /**
     * @brief Blocking. Sets the steering angle
     * Input is clipped to available range
     *
     * @param v -1 to 1 Zero being straight wheels
     */
    virtual void setSteeringAngle(float);
    static constexpr INTEGER32 SteeringAngleRaw_Min = 0x9AB;  // 2475;
    static constexpr INTEGER32 SteeringAngleRaw_Max = 0x1700; // 5888;

    /**
     * @brief Signals that RTD has timed out, used in callback of canfestival
     *
     */
    virtual void signalRTDTimeout()
    {
        _rtdTimeout = true;
    }

    /**
     * @brief Signals that RTD is available again, used in callback of canfestival
     *
     */
    virtual void signalRTDRecovery()
    {
        // Kickstarting rpdo timers will cause this to get called once but we want
        // to know when the rtd is really there so ignore first call
        if (_firstRTDRecoveryCall)
        {
            _firstRTDRecoveryCall = false;
        }
        else
        {
            _rtdTimeout = false;
        }
    }

    virtual bool getRTDTimeout()
    {
        return _rtdTimeout;
    }

    template <typename InputType, typename OutputType>
    static OutputType mapValue(const InputType fromMin, const InputType fromMax,
                               const OutputType toMin, const OutputType toMax,
                               const InputType value)
    {
        // excluding everything not unit tested and not needed with devices
        specialAssert(fromMin < fromMax);
        specialAssert(toMin < toMax);
        specialAssert(!(fromMax == 0 && fromMin < 0));
        specialAssert(!(toMax == 0 && toMin < 0));

        const auto _toMin = static_cast<float>(toMin);
        const auto _toMax = static_cast<float>(toMax);
        const auto _fromMin = static_cast<float>(fromMin);
        const auto _fromMax = static_cast<float>(fromMax);

        const float v{std::min(std::max(static_cast<float>(value), _fromMin), _fromMax)};

        // https://stackoverflow.com/questions/5731863/mapping-a-numeric-range-onto-another
        return static_cast<OutputType>(_toMin + (_toMax - _toMin) *
                                                    ((v - _fromMin) / (_fromMax - _fromMin)));
    }

    static void testHook_signalRTDRecovery(){};
    static void testHook_signalRTDTimeout(){};

    static constexpr size_t MonitoredDeviceCount = 5;
    static constexpr size_t StateControlledDeviceCount = 4;

    virtual std::array<BusDevices, MonitoredDeviceCount> getMonitoredDevices() const final;
    virtual std::array<BusDevices, StateControlledDeviceCount>
    getStateControlledDevices() const final;

    static Message RTD_StateBootupMessage;
    static Message RTD_StateReadyMessage;
    static Message RTD_StateEmcyMessage;

private:
    CanIO &_canIO;
    Logging &_log;
    static Canopen *_instance;

    std::array<MonitoredDevice, MonitoredDeviceCount> _monitoredDevices;
    std::array<StateControlledDevice, StateControlledDeviceCount> _stateControlledDevices;
    std::array<Coupling, 2> _couplings;
    bool _rtdTimeout = true;
    bool _firstRTDRecoveryCall = true;
    std::array<TIMER_HANDLE, MaxRPDOEventTimers> _rpdoTimers;

    // designated initializers are very frowned upon by the
    // compiler so keep this in sync with the definition
    static constexpr uint8_t CouplingIndex_Brake = 0;
    static constexpr uint8_t CouplingIndex_Steering = 1;

    /* Canfestival callbacks */
    static void cbSlaveStateChange(CO_Data *d, UNS8 heartbeatID, e_nodeState state);
    static void cbHeartbeatError(CO_Data *d, UNS8 heartbeatID);
    static void cbSDO(CO_Data *d, UNS8 nodeId);

    /* Support functions for state controlling */
    int8_t findInStateControlledList(const BusDevices device);

    uint8_t convertDeviceState(CanDeviceState);

    /* Support for coupling setting */
    void _setCouplingState(Coupling &, bool state);

    static const char *getBusDeviceName(const BusDevices dev);
    static const char *abortCodeToString(uint32_t abortCode);

    /**
     * @brief Due to oversights in the canopen stack there needs to be
     * some hacking around to get the pdos to transmit again after being disabled
     * Executed after actuator pdos are enabled
     *
     */
    virtual void kickstartPDOTranmission();
};

} // namespace remote_control_device